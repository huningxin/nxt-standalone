// Copyright 2017 The NXT Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tests/NXTTest.h"

#include <cstring>

class BufferMapReadTests : public NXTTest {
    protected:

        static void MapReadCallback(nxtBufferMapAsyncStatus status, const void* data, nxtCallbackUserdata userdata) {
            ASSERT_EQ(NXT_BUFFER_MAP_ASYNC_STATUS_SUCCESS, status);
            ASSERT_NE(nullptr, data);

            auto test = reinterpret_cast<BufferMapReadTests*>(static_cast<uintptr_t>(userdata));
            test->mappedData = data;
        }

        const void* MapReadAsyncAndWait(const nxt::Buffer& buffer, uint32_t start, uint32_t offset) {
            buffer.MapReadAsync(start, offset, MapReadCallback, static_cast<nxt::CallbackUserdata>(reinterpret_cast<uintptr_t>(this)));

            while (mappedData == nullptr) {
                WaitABit();
            }

            return mappedData;
        }

    private:
        const void* mappedData = nullptr;
};

// Test that the simplest map read (one u8 at offset 0) works.
TEST_P(BufferMapReadTests, SmallReadAtZero) {
    nxt::Buffer buffer = device.CreateBufferBuilder()
        .SetSize(1)
        .SetAllowedUsage(nxt::BufferUsageBit::MapRead | nxt::BufferUsageBit::TransferDst)
        .SetInitialUsage(nxt::BufferUsageBit::TransferDst)
        .GetResult();

    uint8_t myData = 187;
    buffer.SetSubData(0, sizeof(myData), &myData);

    buffer.TransitionUsage(nxt::BufferUsageBit::MapRead);
    const void* mappedData = MapReadAsyncAndWait(buffer, 0, 1);
    ASSERT_EQ(myData, *reinterpret_cast<const uint8_t*>(mappedData));

    buffer.Unmap();
}

// Test mapping a buffer at an offset.
TEST_P(BufferMapReadTests, SmallReadAtOffset) {
    nxt::Buffer buffer = device.CreateBufferBuilder()
        .SetSize(4000)
        .SetAllowedUsage(nxt::BufferUsageBit::MapRead | nxt::BufferUsageBit::TransferDst)
        .SetInitialUsage(nxt::BufferUsageBit::TransferDst)
        .GetResult();

    uint8_t myData = 234;
    buffer.SetSubData(2048, sizeof(myData), &myData);

    buffer.TransitionUsage(nxt::BufferUsageBit::MapRead);
    const void* mappedData = MapReadAsyncAndWait(buffer, 2048, 4);
    ASSERT_EQ(myData, *reinterpret_cast<const uint8_t*>(mappedData));

    buffer.Unmap();
}

// Test mapping a buffer at an offset that's not uint32-aligned.
TEST_P(BufferMapReadTests, SmallReadAtUnalignedOffset) {
    nxt::Buffer buffer = device.CreateBufferBuilder()
        .SetSize(4000)
        .SetAllowedUsage(nxt::BufferUsageBit::MapRead | nxt::BufferUsageBit::TransferDst)
        .SetInitialUsage(nxt::BufferUsageBit::TransferDst)
        .GetResult();

    uint8_t myData = 213;
    buffer.SetSubData(3, 1, &myData);

    buffer.TransitionUsage(nxt::BufferUsageBit::MapRead);
    const void* mappedData = MapReadAsyncAndWait(buffer, 3, 1);
    ASSERT_EQ(myData, *reinterpret_cast<const uint8_t*>(mappedData));

    buffer.Unmap();
}

// Test mapping large ranges of a buffer.
TEST_P(BufferMapReadTests, LargeRead) {
    constexpr uint32_t kDataSize = 1000 * 1000;
    std::vector<uint32_t> myData;
    for (uint32_t i = 0; i < kDataSize; ++i) {
        myData.push_back(i);
    }

    nxt::Buffer buffer = device.CreateBufferBuilder()
        .SetSize(static_cast<uint32_t>(kDataSize * sizeof(uint32_t)))
        .SetAllowedUsage(nxt::BufferUsageBit::MapRead | nxt::BufferUsageBit::TransferDst)
        .SetInitialUsage(nxt::BufferUsageBit::TransferDst)
        .GetResult();

    buffer.SetSubData(0, kDataSize * sizeof(uint32_t), reinterpret_cast<uint8_t*>(myData.data()));

    buffer.TransitionUsage(nxt::BufferUsageBit::MapRead);
    const void* mappedData = MapReadAsyncAndWait(buffer, 0, static_cast<uint32_t>(kDataSize * sizeof(uint32_t)));
    ASSERT_EQ(0, memcmp(mappedData, myData.data(), kDataSize * sizeof(uint32_t)));

    buffer.Unmap();
}

NXT_INSTANTIATE_TEST(BufferMapReadTests, D3D12Backend, MetalBackend, OpenGLBackend, VulkanBackend)

class BufferMapWriteTests : public NXTTest {
    protected:

        static void MapWriteCallback(nxtBufferMapAsyncStatus status, void* data, nxtCallbackUserdata userdata) {
            ASSERT_EQ(NXT_BUFFER_MAP_ASYNC_STATUS_SUCCESS, status);
            ASSERT_NE(nullptr, data);

            auto test = reinterpret_cast<BufferMapWriteTests*>(static_cast<uintptr_t>(userdata));
            test->mappedData = data;
        }

        void* MapWriteAsyncAndWait(const nxt::Buffer& buffer, uint32_t start, uint32_t offset) {
            buffer.MapWriteAsync(start, offset, MapWriteCallback, static_cast<nxt::CallbackUserdata>(reinterpret_cast<uintptr_t>(this)));

            while (mappedData == nullptr) {
                WaitABit();
            }

            return mappedData;
        }

    private:
        void* mappedData = nullptr;
};

// Test that the simplest map write (one u32 at offset 0) works.
TEST_P(BufferMapWriteTests, SmallWriteAtZero) {
    nxt::Buffer buffer = device.CreateBufferBuilder()
        .SetSize(4)
        .SetAllowedUsage(nxt::BufferUsageBit::MapWrite | nxt::BufferUsageBit::TransferSrc)
        .SetInitialUsage(nxt::BufferUsageBit::MapWrite)
        .GetResult();

    uint32_t myData = 2934875;
    void* mappedData = MapWriteAsyncAndWait(buffer, 0, 4);
    memcpy(mappedData, &myData, sizeof(myData));
    buffer.Unmap();

    EXPECT_BUFFER_U32_EQ(myData, buffer, 0);
}

// Test mapping a buffer at an offset.
TEST_P(BufferMapWriteTests, SmallWriteAtOffset) {
    nxt::Buffer buffer = device.CreateBufferBuilder()
        .SetSize(4000)
        .SetAllowedUsage(nxt::BufferUsageBit::MapWrite | nxt::BufferUsageBit::TransferSrc)
        .SetInitialUsage(nxt::BufferUsageBit::MapWrite)
        .GetResult();

    uint32_t myData = 2934875;
    void* mappedData = MapWriteAsyncAndWait(buffer, 2048, 4);
    memcpy(mappedData, &myData, sizeof(myData));
    buffer.Unmap();

    EXPECT_BUFFER_U32_EQ(myData, buffer, 2048);
}

// Test mapping large ranges of a buffer.
TEST_P(BufferMapWriteTests, LargeWrite) {
    constexpr uint32_t kDataSize = 1000 * 1000;
    std::vector<uint32_t> myData;
    for (uint32_t i = 0; i < kDataSize; ++i) {
        myData.push_back(i);
    }

    nxt::Buffer buffer = device.CreateBufferBuilder()
        .SetSize(static_cast<uint32_t>(kDataSize * sizeof(uint32_t)))
        .SetAllowedUsage(nxt::BufferUsageBit::MapWrite | nxt::BufferUsageBit::TransferSrc)
        .SetInitialUsage(nxt::BufferUsageBit::MapWrite)
        .GetResult();

    void* mappedData = MapWriteAsyncAndWait(buffer, 0, kDataSize * sizeof(uint32_t));
    memcpy(mappedData, myData.data(), kDataSize * sizeof(uint32_t));
    buffer.Unmap();

    EXPECT_BUFFER_U32_RANGE_EQ(myData.data(), buffer, 0, kDataSize);
}

NXT_INSTANTIATE_TEST(BufferMapWriteTests, D3D12Backend, MetalBackend, OpenGLBackend, VulkanBackend)

class BufferSetSubDataTests : public NXTTest {
};

// Test the simplest set sub data: setting one u8 at offset 0.
TEST_P(BufferSetSubDataTests, SmallDataAtZero) {
    nxt::Buffer buffer = device.CreateBufferBuilder()
        .SetSize(1)
        .SetAllowedUsage(nxt::BufferUsageBit::TransferSrc | nxt::BufferUsageBit::TransferDst)
        .SetInitialUsage(nxt::BufferUsageBit::TransferDst)
        .GetResult();

    uint8_t value = 171;
    buffer.SetSubData(0, sizeof(value), &value);

    EXPECT_BUFFER_U8_EQ(value, buffer, 0);
}

// Test that SetSubData offset works.
TEST_P(BufferSetSubDataTests, SmallDataAtOffset) {
    nxt::Buffer buffer = device.CreateBufferBuilder()
        .SetSize(4000)
        .SetAllowedUsage(nxt::BufferUsageBit::TransferSrc | nxt::BufferUsageBit::TransferDst)
        .SetInitialUsage(nxt::BufferUsageBit::TransferDst)
        .GetResult();

    constexpr uint32_t kOffset = 2000;
    uint8_t value = 231;
    buffer.SetSubData(kOffset, sizeof(value), &value);

    EXPECT_BUFFER_U8_EQ(value, buffer, kOffset);
}

// Stress test for many calls to SetSubData
TEST_P(BufferSetSubDataTests, ManySetSubData) {
    if (IsD3D12() || IsMetal() || IsVulkan()) {
        // TODO(cwallez@chromium.org): Use ringbuffers for SetSubData on explicit APIs.
        // otherwise this creates too many resources and can take freeze the driver(?)
        std::cout << "Test skipped on D3D12, Metal and Vulkan" << std::endl;
        return;
    }

    constexpr uint32_t kSize = 4000 * 1000;
    constexpr uint32_t kElements = 1000 * 1000;
    nxt::Buffer buffer = device.CreateBufferBuilder()
        .SetSize(kSize)
        .SetAllowedUsage(nxt::BufferUsageBit::TransferSrc | nxt::BufferUsageBit::TransferDst)
        .SetInitialUsage(nxt::BufferUsageBit::TransferDst)
        .GetResult();

    std::vector<uint32_t> expectedData;
    for (uint32_t i = 0; i < kElements; ++i) {
        buffer.SetSubData(i * sizeof(uint32_t), sizeof(i), reinterpret_cast<uint8_t*>(&i));
        expectedData.push_back(i);
    }

    EXPECT_BUFFER_U32_RANGE_EQ(expectedData.data(), buffer, 0, kElements);
}

// Test using SetSubData for lots of data
TEST_P(BufferSetSubDataTests, LargeSetSubData) {
    constexpr uint32_t kSize = 4000 * 1000;
    constexpr uint32_t kElements = 1000 * 1000;
    nxt::Buffer buffer = device.CreateBufferBuilder()
        .SetSize(kSize)
        .SetAllowedUsage(nxt::BufferUsageBit::TransferSrc | nxt::BufferUsageBit::TransferDst)
        .SetInitialUsage(nxt::BufferUsageBit::TransferDst)
        .GetResult();

    std::vector<uint32_t> expectedData;
    for (uint32_t i = 0; i < kElements; ++i) {
        expectedData.push_back(i);
    }

    buffer.SetSubData(0, kElements * sizeof(uint32_t), reinterpret_cast<uint8_t*>(expectedData.data()));

    EXPECT_BUFFER_U32_RANGE_EQ(expectedData.data(), buffer, 0, kElements);
}

NXT_INSTANTIATE_TEST(BufferSetSubDataTests,
                     D3D12Backend,
                     MetalBackend,
                     OpenGLBackend,
                     VulkanBackend)
