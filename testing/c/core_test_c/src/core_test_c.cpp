/* ========================= eCAL LICENSE =================================
 *
 * Copyright (C) 2016 - 2019 Continental Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ========================= eCAL LICENSE =================================
*/

#include <ecal/ecalc.h>
#include <ecal/cimpl/ecal_core_cimpl.h>
#include <ecal/msg/string/publisher.h>
#include <ecal/msg/string/subscriber.h>
#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

#define CMN_REGISTRATION_REFRESH   1000

namespace {
    // subscriber callback function
    void OnReceive(long long clock_)
    {
        static long long accumulated_clock = 0;
        accumulated_clock += clock_;
    }

#if 0
    // timer callback function
  std::atomic_size_t     g_callback_received{ 0 };
  std::vector<long long> g_timer_vec(100);
  void OnTimer()
  {
    if (g_callback_received < g_timer_vec.size()) g_timer_vec[g_callback_received] = eCAL::Time::GetMicroSeconds();
    g_callback_received += 1;
  }
#endif
}

TEST(CoreC, CheckIfIsInitialized)
{
// check if initialized
EXPECT_EQ(0, eCAL_IsInitialized(0));
// initialize eCAL API
eCAL_Initialize(0, nullptr, "check if is initialized", 0);
// check if initialized
EXPECT_EQ(1, eCAL_IsInitialized(0));
// finalize eCAL API
eCAL_Finalize(0);
// check if initialized
EXPECT_EQ(0, eCAL_IsInitialized(0));
}

TEST(CoreC, MultipleInitializeFinalize)
{
// try to initialize / finalize multiple times
for (auto i = 0; i < 4; ++i)
{
// initialize eCAL API
EXPECT_EQ(0, eCAL_Initialize(0, nullptr, "multiple initialize/finalize", 0));

// finalize eCAL API
EXPECT_EQ(0, eCAL_Finalize(0));
}
}

TEST(CoreC, LeakedPubSub)
{
// initialize eCAL API
EXPECT_EQ(0, eCAL_Initialize(0, nullptr, "leaked pub/sub", 0));

// enable loop back communication in the same thread
eCAL_Util_EnableLoopback(1);

// create subscriber and register a callback
eCAL::string::CSubscriber<std::string> sub("foo");
sub.AddReceiveCallback(std::bind(OnReceive, std::placeholders::_4));

// create publisher
eCAL::string::CPublisher<std::string> pub("foo");

// let's match them
eCAL_Process_SleepMS(2 * CMN_REGISTRATION_REFRESH);

// start publishing thread
std::atomic<bool> pub_stop(false);
std::thread pub_t([&](){
    while (!pub_stop)
    {
        pub.Send("Hello World");
#if 0
        // some kind of busy waiting....
            int y = 0;
            for (int i = 0; i < 100000; i++)
                {
                y += i;
                }
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
    }
});

// let them work together
std::this_thread::sleep_for(std::chrono::seconds(2));

// finalize eCAL API
// without destroying any pub / sub
EXPECT_EQ(0, eCAL_Finalize(0));

// stop publishing thread
pub_stop = true; pub_t.join();
}

TEST(CoreC, CallbackDestruction)
{
// initialize eCAL API
EXPECT_EQ(0, eCAL_Initialize(0, nullptr, "callback destruction", 0));

// enable loop back communication in the same thread
eCAL_Util_EnableLoopback(1);

// create subscriber and register a callback
std::shared_ptr<eCAL::string::CSubscriber<std::string>> sub;

// create publisher
eCAL::string::CPublisher<std::string> pub("foo");

// start publishing thread
std::atomic<bool> pub_stop(false);
std::thread pub_t([&](){
    while (!pub_stop)
    {
        pub.Send("Hello World");
#if 0
        // some kind of busy waiting....
            int y = 0;
            for (int i = 0; i < 100000; i++)
                {
                y += i;
                }
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
    }
});

std::atomic<bool> sub_stop(false);
std::thread sub_t([&](){
    while (!sub_stop)
    {
        sub = std::make_shared<eCAL::string::CSubscriber<std::string>>("foo");
        sub->AddReceiveCallback(std::bind(OnReceive, std::placeholders::_4));
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
});

// let them work together
std::this_thread::sleep_for(std::chrono::seconds(10));

// stop publishing thread
pub_stop = true;
pub_t.join();

sub_stop = true;
sub_t.join();

// finalize eCAL API
// without destroying any pub / sub
EXPECT_EQ(0, eCAL_Finalize(0));
}


