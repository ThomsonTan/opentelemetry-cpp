// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#define ENABLE_LOGS_PREVIEW 1
#ifdef ENABLE_LOGS_PREVIEW

#  include "opentelemetry/common/timestamp.h"
#  include "opentelemetry/logs/logger.h"
#  include "opentelemetry/logs/provider.h"
#  include "opentelemetry/nostd/shared_ptr.h"

#  include <condition_variable>
#  include <thread>
#  include <mutex>
#  include <vector>

#  include <benchmark/benchmark.h>

using opentelemetry::logs::EventId;
using opentelemetry::logs::Logger;
using opentelemetry::logs::LoggerProvider;
using opentelemetry::logs::Provider;
using opentelemetry::logs::Severity;
using opentelemetry::nostd::shared_ptr;
using opentelemetry::nostd::span;
using opentelemetry::nostd::string_view;

namespace common  = opentelemetry::common;
namespace nostd   = opentelemetry::nostd;
namespace trace   = opentelemetry::trace;
namespace log_api = opentelemetry::logs;

namespace
{

class Barrier {
public:
    explicit Barrier(std::size_t iCount) : 
      mThreshold(iCount), 
      mCount(iCount), 
      mGeneration(0) {
    }

    void Wait() {
        std::unique_lock<std::mutex> lLock{mMutex};
        auto lGen = mGeneration;
        if (!--mCount) {
            mGeneration++;
            mCount = mThreshold;
            mCond.notify_all();
        } else {
            mCond.wait(lLock, [this, lGen] { return lGen != mGeneration; });
        }
    }

private:
    std::mutex mMutex;
    std::condition_variable mCond;
    std::size_t mThreshold;
    std::size_t mCount;
    std::size_t mGeneration;
};

static void ThreadRoutine(Barrier &barrier, benchmark::State &state, int thread_id, std::function<void(benchmark::State &state)> func)
{
    barrier.Wait();

    if (thread_id == 0) {
        state.ResumeTiming();
    }

    barrier.Wait();

    for (int64_t i = 0; i < 0x100000000ull; i++) {
        func(state);
    }

    if (thread_id == 0) {
        state.PauseTiming();
    }

    barrier.Wait();
}

void MultiThreadRunner(benchmark::State& state, std::function<void(benchmark::State &state)> func)
{
    int num_threads = std::thread::hardware_concurrency();

    Barrier barrier(num_threads);

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(ThreadRoutine, std::ref(barrier), std::ref(state), i, func);
    }

    for (auto &thread : threads) {
        thread.join();
    }
}


static void BM_LoggerSingleCall_UnstructuredLog(benchmark::State &state)
{
    auto lp = Provider::GetLoggerProvider();
    auto logger = lp->GetLogger("UnstructuredLog");

    for (auto _ : state) {
        state.PauseTiming();

        MultiThreadRunner(state, [&logger](benchmark::State &state) {
            logger->Trace("This is a simple unstructured log message");
            return 0;
        });

        state.ResumeTiming();
    }
}
BENCHMARK(BM_LoggerSingleCall_UnstructuredLog);

static void BM_LoggerSingleCall_StructuredLog(benchmark::State &state)
{
    auto lp = Provider::GetLoggerProvider();
    auto logger = lp->GetLogger("StructuredLog");

    for (auto _ : state) {
        state.PauseTiming();

        MultiThreadRunner(state, [&logger](benchmark::State &state) {
            logger->Trace("This is a simple structured log message from {process_id}:{thread_id}",
                        opentelemetry::common::MakeAttributes({{"process_id", 12347}, {"thread_id", 12348}}));
            return 0;
        });

        state.ResumeTiming();
    }
}
BENCHMARK(BM_LoggerSingleCall_StructuredLog);

static void BM_LoggerSingleCall_StructuredLogWithEventId(benchmark::State &state)
{
    auto lp = Provider::GetLoggerProvider();
    auto logger = lp->GetLogger("StructuredLogWithEventId");

    for (auto _ : state) {
        state.PauseTiming();

        MultiThreadRunner(state, [&logger](benchmark::State &state) {
            logger->Trace("This is a simple structured log message from {process_id}:{thread_id}",
                        opentelemetry::common::MakeAttributes({{"process_id", 12347}, {"thread_id", 12348}}));
            logger->Trace(0x1234567890, "This is a simple structured log message from {process_id}:{thread_id}",
                        opentelemetry::common::MakeAttributes({{"process_id", 12347}, {"thread_id", 12348}}));
            return 0;
        });

        state.ResumeTiming();
    }
}
BENCHMARK(BM_LoggerSingleCall_StructuredLogWithEventId);

static void BM_LoggerSingleCall_StructuredLogWithEventIdStruct(benchmark::State &state)
{
    auto lp = Provider::GetLoggerProvider();
    auto logger = lp->GetLogger("StructuredLogWithEventId");

    const EventId function_name_event_id{0x12345678, "Company.Component.SubComponent.FunctionName"};

    for (auto _ : state) {
        state.PauseTiming();

        MultiThreadRunner(state, [&logger, &function_name_event_id](benchmark::State &state) {
            logger->Trace(function_name_event_id, "Simulate function enter trace message from {process_id}:{thread_id}",
                        opentelemetry::common::MakeAttributes({{"process_id", 12347}, {"thread_id", 12348}}));
            logger->Trace(function_name_event_id, "Simulate function enter trace message from {process_id}:{thread_id}",
                        opentelemetry::common::MakeAttributes({{"process_id", 12347}, {"thread_id", 12348}}));
            return 0;
        });

        state.ResumeTiming();
    }
}
BENCHMARK(BM_LoggerSingleCall_StructuredLogWithEventIdStruct);

class LoggerFixture : public benchmark::Fixture
{
  public:
    void SetUp(const ::benchmark::State &state)
    {
        auto lp = Provider::GetLoggerProvider();
        logger = lp->GetLogger("StructuredLogWithEventId");
    }

    void TearDown(const ::benchmark::State &state) {}

    // test methods
    int FunctionWithUnstructuredLog(benchmark::State &state);

    int FunctionWithStructuredLog(benchmark::State &state);

    int FunctionWithStructuredLogAndEventId(benchmark::State &state);

    int FunctionWithStructuredLogAndEventIdStruct(benchmark::State &state);

    int FunctionWithStructuredLogAndEventIdStructCheckEnabled(benchmark::State &state);

private:
    nostd::shared_ptr<log_api::Logger> logger;
    const EventId function_enter_event_id{0x12345678, "Company.Component.SubComponent.FunctionEnter"};
    const EventId function_exit_event_id{0x12345679, "Company.Component.SubComponent.FunctionExit"};
};

int LoggerFixture::FunctionWithUnstructuredLog(benchmark::State &state)
{
    logger->Trace("This is a simple unstructured log message");

    logger->Trace("This is another simple unstructured log message");
    return 0;
}

int LoggerFixture::FunctionWithStructuredLog(benchmark::State &state)
{
    logger->Trace("This is a simple structured log message from {process_id}:{thread_id}",
                  opentelemetry::common::MakeAttributes({{"process_id", 12347}, {"thread_id", 12348}}));
                
    logger->Trace("This is another simple structured log message from {process_id}:{thread_id}",
                  opentelemetry::common::MakeAttributes({{"process_id", 12347}, {"thread_id", 12348}}));
    return 0;
}

int LoggerFixture::FunctionWithStructuredLogAndEventId(benchmark::State &state)
{
    logger->Trace(0x1234567890, "This is a simple structured log message from {process_id}:{thread_id}",
                  opentelemetry::common::MakeAttributes({{"process_id", 12347}, {"thread_id", 12348}}));

    logger->Trace(0x123456789a, "This is a simple structured log message from {process_id}:{thread_id}",
                  opentelemetry::common::MakeAttributes({{"process_id", 12347}, {"thread_id", 12348}}));

    return 0;
}

int LoggerFixture::FunctionWithStructuredLogAndEventIdStruct(benchmark::State &state)
{
  logger->Trace(function_enter_event_id, "Simulate function enter trace message from {process_id}:{thread_id}",
                opentelemetry::common::MakeAttributes({{"process_id", 12347}, {"thread_id", 12348}}));

  logger->Trace(function_exit_event_id, "Simulate function exit trace message from {process_id}:{thread_id}",
                opentelemetry::common::MakeAttributes({{"process_id", 12347}, {"thread_id", 12348}}));
  return 0;
}

int LoggerFixture::FunctionWithStructuredLogAndEventIdStructCheckEnabled(benchmark::State &state)
{
  if (logger->Enabled(Severity::kTrace, function_enter_event_id)) {
      logger->Trace(function_enter_event_id, "Simulate function enter trace message from {process_id}:{thread_id}",
                    opentelemetry::common::MakeAttributes({{"process_id", 12347}, {"thread_id", 12348}}));
  }

  if (logger->Enabled(Severity::kTrace, function_exit_event_id)) {
      logger->Trace(function_exit_event_id, "Simulate function exit trace message from {process_id}:{thread_id}",
                    opentelemetry::common::MakeAttributes({{"process_id", 12347}, {"thread_id", 12348}}));
  }

  return 0;
}

BENCHMARK_DEFINE_F(LoggerFixture, BM_LoggerFunctionWithUnstructuredLog)(benchmark::State &state)
{
    for (auto _ : state) {
        state.PauseTiming();

        MultiThreadRunner(state, [this](benchmark::State &state){
            FunctionWithUnstructuredLog(state);
        });

        state.ResumeTiming();
    }
}

BENCHMARK_REGISTER_F(LoggerFixture, BM_LoggerFunctionWithUnstructuredLog);

BENCHMARK_DEFINE_F(LoggerFixture, BM_LoggerFunctionWithStructuredLog)(benchmark::State &state)
{
    for (auto _ : state) {
        state.PauseTiming();

        MultiThreadRunner(state, [this](benchmark::State &state){
            FunctionWithStructuredLog(state);
        });

        state.ResumeTiming();
    }
}

BENCHMARK_REGISTER_F(LoggerFixture, BM_LoggerFunctionWithStructuredLog);

BENCHMARK_DEFINE_F(LoggerFixture, BM_LoggerFunctionWithStructuredLogAndEventId)(benchmark::State &state)
{
    for (auto _ : state) {
        state.PauseTiming();

        MultiThreadRunner(state, [this](benchmark::State &state){
            FunctionWithStructuredLogAndEventId(state);
        });

        state.ResumeTiming();
    }
}

BENCHMARK_REGISTER_F(LoggerFixture, BM_LoggerFunctionWithStructuredLogAndEventId);

BENCHMARK_DEFINE_F(LoggerFixture, BM_LoggerFunctionWithStructuredLogAndEventIdStruct)(benchmark::State &state)
{
    for (auto _ : state) {
        state.PauseTiming();

        MultiThreadRunner(state, [this](benchmark::State &state){
            FunctionWithStructuredLogAndEventIdStruct(state);
        });

        state.ResumeTiming();
    }
}

BENCHMARK_REGISTER_F(LoggerFixture, BM_LoggerFunctionWithStructuredLogAndEventIdStruct);

BENCHMARK_DEFINE_F(LoggerFixture, BM_LoggerFunctionWithStructuredLogAndEventIdStructCheckEnabled)(benchmark::State &state)
{
    for (auto _ : state) {
        state.PauseTiming();

        MultiThreadRunner(state, [this](benchmark::State &state){
            FunctionWithStructuredLogAndEventIdStructCheckEnabled(state);
        });

        state.ResumeTiming();
    }
}

BENCHMARK_REGISTER_F(LoggerFixture, BM_LoggerFunctionWithStructuredLogAndEventIdStructCheckEnabled);
}

int main(int argc, char **argv)
{
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}

#endif
