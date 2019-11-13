/*
 * Copyright 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "manager.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace blobs
{

void BlobManager::incrementOpen(const std::string& path)
{
    if (path.empty())
    {
        return;
    }

    openFiles[path] += 1;
}

void BlobManager::decrementOpen(const std::string& path)
{
    if (path.empty())
    {
        return;
    }

    /* TODO(venture): Check into the iterator from find, does it makes sense
     * to just update it directly? */
    auto entry = openFiles.find(path);
    if (entry != openFiles.end())
    {
        /* found it, decrement it and remove it if 0. */
        openFiles[path] -= 1;
        if (openFiles[path] == 0)
        {
            openFiles.erase(path);
        }
    }
}

int BlobManager::getOpen(const std::string& path) const
{
    /* No need to input check on the read-only call. */
    auto entry = openFiles.find(path);
    if (entry != openFiles.end())
    {
        return entry->second;
    }

    return 0;
}

bool BlobManager::registerHandler(std::unique_ptr<GenericBlobInterface> handler)
{
    if (!handler)
    {
        return false;
    }

    handlers.push_back(std::move(handler));
    return true;
}

uint32_t BlobManager::buildBlobList()
{
    /* Clear out the current list (IPMI handler is presently single-threaded).
     */
    ids.clear();

    /* Grab the list of blobs and extend the local list */
    for (const auto& h : handlers)
    {
        std::vector<std::string> blobs = h->getBlobIds();
        ids.insert(ids.end(), blobs.begin(), blobs.end());
    }

    return ids.size();
}

std::string BlobManager::getBlobId(uint32_t index)
{
    /* Range check. */
    if (index >= ids.size())
    {
        return "";
    }

    return ids[index];
}

bool BlobManager::open(uint16_t flags, const std::string& path,
                       uint16_t* session)
{
    GenericBlobInterface* handler = getHandler(path);

    /* No handler found. */
    if (!handler)
    {
        return false;
    }

    /* No sessions available... */
    if (!getSession(session))
    {
        return false;
    }

    /* Verify flags - must be at least read or write */
    if (!(flags & (OpenFlags::read | OpenFlags::write)))
    {
        /* Neither read not write set, which means calls to Read/Write will
         * reject. */
        return false;
    }

    if (!handler->open(*session, flags, path))
    {
        return false;
    }

    /* Associate session with handler */
    sessions[*session] = SessionInfo(path, handler, flags);
    openSessions[handler].insert(*session);
    incrementOpen(path);
    return true;
}

GenericBlobInterface* BlobManager::getHandler(const std::string& path)
{
    /* Find a handler. */
    auto h = std::find_if(
        handlers.begin(), handlers.end(),
        [&path](const auto& iter) { return (iter->canHandleBlob(path)); });
    if (h != handlers.end())
    {
        return h->get();
    }

    return nullptr;
}

GenericBlobInterface* BlobManager::getHandler(uint16_t session)
{
    auto item = sessions.find(session);
    if (item == sessions.end())
    {
        return nullptr;
    }

    return item->second.handler;
}

SessionInfo* BlobManager::getSessionInfo(uint16_t session)
{
    auto item = sessions.find(session);
    if (item == sessions.end())
    {
        return nullptr;
    }

    /* If we go to multi-threaded, this pointer can be invalidated and this
     * method will need to change.
     */
    return &item->second;
}

std::string BlobManager::getPath(uint16_t session) const
{
    auto item = sessions.find(session);
    if (item == sessions.end())
    {
        return "";
    }

    return item->second.blobId;
}

bool BlobManager::stat(const std::string& path, BlobMeta* meta)
{
    /* meta should never be NULL. */
    GenericBlobInterface* handler = getHandler(path);

    /* No handler found. */
    if (!handler)
    {
        return false;
    }

    return handler->stat(path, meta);
}

bool BlobManager::stat(uint16_t session, BlobMeta* meta)
{
    /* meta should never be NULL. */
    GenericBlobInterface* handler = getHandler(session);

    /* No handler found. */
    if (!handler)
    {
        return false;
    }

    return handler->stat(session, meta);
}

bool BlobManager::commit(uint16_t session, const std::vector<uint8_t>& data)
{
    GenericBlobInterface* handler = getHandler(session);

    /* No handler found. */
    if (!handler)
    {
        return false;
    }

    return handler->commit(session, data);
}

bool BlobManager::close(uint16_t session)
{
    GenericBlobInterface* handler = getHandler(session);

    /* No handler found. */
    if (!handler)
    {
        return false;
    }

    /* Handler returns failure */
    if (!handler->close(session))
    {
        return false;
    }

    sessions.erase(session);
    openSessions[handler].erase(session);
    if (openSessions[handler].size() == 0)
    {
        openSessions.erase(handler);
    }
    decrementOpen(getPath(session));
    return true;
}

std::vector<uint8_t> BlobManager::read(uint16_t session, uint32_t offset,
                                       uint32_t requestedSize)
{
    SessionInfo* info = getSessionInfo(session);

    /* No session found. */
    if (!info)
    {
        return std::vector<uint8_t>();
    }

    /* Check flags. */
    if (!(info->flags & OpenFlags::read))
    {
        return std::vector<uint8_t>();
    }

    /* TODO: Currently, configure_ac isn't finding libuserlayer, w.r.t the
     * symbols I need.
     */

    /** The channel to use for now.
     * TODO: We will receive this information through the IPMI message call.
     */
    // const int ipmiChannel = ipmi::currentChNum;
    /** This information is transport specific.
     * TODO: We need a way to know this dynamically.
     * on BT, 4 bytes of header, and 1 reply code.
     */
    // uint32_t maxTransportSize = ipmi::getChannelMaxTransferSize(ipmiChannel);

    /* Try reading from it. */
    return info->handler->read(session, offset,
                               std::min(maximumReadSize, requestedSize));
}

bool BlobManager::write(uint16_t session, uint32_t offset,
                        const std::vector<uint8_t>& data)
{
    SessionInfo* info = getSessionInfo(session);

    /* No session found. */
    if (!info)
    {
        return false;
    }

    /* Check flags. */
    if (!(info->flags & OpenFlags::write))
    {
        return false;
    }

    /* Try writing to it. */
    return info->handler->write(session, offset, data);
}

bool BlobManager::deleteBlob(const std::string& path)
{
    GenericBlobInterface* handler = getHandler(path);

    /* No handler found. */
    if (!handler)
    {
        return false;
    }

    /* Check if the file has any open handles. */
    if (getOpen(path) > 0)
    {
        return false;
    }

    /* Try deleting it. */
    return handler->deleteBlob(path);
}

bool BlobManager::writeMeta(uint16_t session, uint32_t offset,
                            const std::vector<uint8_t>& data)
{
    SessionInfo* info = getSessionInfo(session);

    /* No session found. */
    if (!info)
    {
        return false;
    }

    /* Try writing metadata to it. */
    return info->handler->writeMeta(session, offset, data);
}

bool BlobManager::getSession(uint16_t* sess)
{
    uint16_t tries = 0;

    if (!sess)
    {
        return false;
    }

    /* This is not meant to fail as you have 64KiB values available. */

    /* TODO(venture): We could just count the keys in the session map to know
     * if it's full.
     */
    do
    {
        uint16_t lsess = next++;
        if (!sessions.count(lsess))
        {
            /* value not in use, return it. */
            (*sess) = lsess;
            return true;
        }
    } while (++tries < 0xffff);

    return false;
}

static std::unique_ptr<BlobManager> manager;

ManagerInterface* getBlobManager()
{
    if (manager == nullptr)
    {
        manager = std::make_unique<BlobManager>();
    }

    return manager.get();
}

} // namespace blobs
