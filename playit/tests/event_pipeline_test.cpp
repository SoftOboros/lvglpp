// event_pipeline_test.cpp — PLAYIT-02 acceptance: NullPipeline
// passes events through unchanged and tick() yields no output.

#include "lvglpp/playit/event_pipeline.hpp"
#include "lvglpp/core/event.hpp"

#include <cassert>
#include <variant>

using namespace lvglpp::playit;
namespace lc = lvglpp::core;

namespace {

void test_null_pipeline_process_passthrough() {
    NullPipeline p;
    lc::Event input{lc::event::PressRelease{42, 24}};

    PipelineOutput out = p.process(input);
    assert(out.primary.has_value());
    assert(!out.secondary.has_value());

    auto* pr = std::get_if<lc::event::PressRelease>(&out.primary.value());
    assert(pr != nullptr);
    assert(pr->x == 42 && pr->y == 24);
}

void test_null_pipeline_tick_is_quiet() {
    NullPipeline p;
    PipelineOutput out = p.tick();
    assert(!out.primary.has_value());
    assert(!out.secondary.has_value());
}

// Custom pipeline that fans out: turns a Tick into PressDown +
// PressRelease (artificial — used only to exercise the secondary
// slot of PipelineOutput).
struct FanOutPipeline final : EventPipeline {
    [[nodiscard]] PipelineOutput process(const lc::Event& event) override {
        if (std::holds_alternative<lc::event::Tick>(event)) {
            return PipelineOutput{
                lc::Event{lc::event::PressDown{1, 2}},
                lc::Event{lc::event::PressRelease{3, 4}},
            };
        }
        return PipelineOutput{event, std::nullopt};
    }
    [[nodiscard]] PipelineOutput tick() override {
        return PipelineOutput{std::nullopt, std::nullopt};
    }
};

void test_pipeline_secondary_slot() {
    FanOutPipeline p;
    lc::Event tick{lc::event::Tick{}};
    PipelineOutput out = p.process(tick);

    assert(out.primary.has_value());
    assert(out.secondary.has_value());

    auto* pd = std::get_if<lc::event::PressDown>(&out.primary.value());
    auto* pr = std::get_if<lc::event::PressRelease>(&out.secondary.value());
    assert(pd && pd->x == 1 && pd->y == 2);
    assert(pr && pr->x == 3 && pr->y == 4);
}

}  // namespace

int main() {
    test_null_pipeline_process_passthrough();
    test_null_pipeline_tick_is_quiet();
    test_pipeline_secondary_slot();
    return 0;
}
