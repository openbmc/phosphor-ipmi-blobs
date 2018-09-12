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

#include "crc.hpp"

namespace blobs
{

void Crc16::clear()
{
    value = crc16Initial;
}

// Origin: security/crypta/ipmi/portable/ipmi_utils.c
void Crc16::compute(const uint8_t* bytes, uint32_t length)
{
    if (!bytes)
    {
        return;
    }

    const int kExtraRounds = 2;
    const uint16_t kLeftBit = 0x8000;
    uint16_t crc = value;
    size_t i, j;

    for (i = 0; i < length + kExtraRounds; ++i)
    {
        for (j = 0; j < 8; ++j)
        {
            bool xor_flag = crc & kLeftBit;
            crc <<= 1;
            // If this isn't an extra round and the current byte's j'th bit
            // from the left is set, increment the CRC.
            if (i < length && bytes[i] & (1 << (7 - j)))
            {
                crc++;
            }
            if (xor_flag)
            {
                crc ^= crc16Ccitt;
            }
        }
    }

    value = crc;
}

uint16_t Crc16::get() const
{
    return value;
}
} // namespace blobs
