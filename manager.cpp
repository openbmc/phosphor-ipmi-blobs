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

void BlobManager::eraseSession(GenericBlobInterface* handler, uint16_t session)
{
    sessions.erase(session);
    /* Ok for openSessions[handler] to be an empty set */
    openSessions[handler].erase(session);

    auto path = getPath(session);
    openFiles[path]--;
    if (openFiles[path] == 0)
    {
        openFiles.erase(path);
    }
}

void BlobManager::cleanUpStaleSessions(GenericBlobInterface* handler)
{
    if (openSessions.count(handler) == 0)
    {
        return;
    }
    auto timeNow = std::chrono::steady_clock::now();

    std::set<uint16_t> expiredSet;
    std::copy_if(openSessions[handler].begin(), openSessions[handler].end(),
                 std::inserter(expiredSet, expiredSet.begin()),
                 [&](auto sessionId) {
                     return timeNow - sessions[sessionId].lastActionTime >=
                            sessionTimeout;
                 });

    for (const auto& session : expiredSet)
    {
        // TODO: The expire call is unimplemented in some handlers. Do we still
        // want to erase the sessions if expire() returns false?
        (void)handler->expire(session);
        eraseSession(handler, session);
    }
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

    /* Try to clean up anything that's falling out of cleanup timeout for this
     * handler */
    cleanUpStaleSessions(handler);

    if (!handler->open(*session, flags, path))
    {
        return false;
    }

    /* Associate session with handler */
    sessions[*session] = SessionInfo(path, handler, flags);
    openSessions[handler].insert(*session);
    openFiles[path]++;
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
    if (auto handler = getActionHandle(session))
    {
        return handler->stat(session, meta);
    }
    return false;
}

bool BlobManager::commit(uint16_t session, const std::vector<uint8_t>& data)
{
    if (auto handler = getActionHandle(session))
    {
        return handler->commit(session, data);
    }
    return false;
}

bool BlobManager::close(uint16_t session)
{
    if (auto handler = getActionHandle(session))
    {
        if (!handler->close(session))
        {
            return false;
        }
        eraseSession(handler, session);
        return true;
    }
    return false;
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

    info->lastActionTime = std::chrono::steady_clock::now();
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

    info->lastActionTime = std::chrono::steady_clock::now();
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
    if (openFiles[path] > 0)
    {
        return false;
    }

    /* Try deleting it. */
    return handler->deleteBlob(path);
}

bool BlobManager::writeMeta(uint16_t session, uint32_t offset,
                            const std::vector<uint8_t>& data)
{
    if (auto handler = getActionHandle(session))
    {
        return handler->writeMeta(session, offset, data);
    }
    return false;
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
