#pragma once

#include "shield/MaskingEngine.hpp"
#include <spdlog/sinks/sink.h>
#include <mutex>
#include <memory>
#include <string>

namespace shield {
namespace integration {

/**
 * @brief A proxy sink for spdlog that masks sensitive data before forwarding
 *        the log message to the actual underlying sink.
 */
class SpdlogMaskingSink : public spdlog::sinks::sink {
public:
    SpdlogMaskingSink(std::shared_ptr<shield::MaskingEngine> engine,
                      spdlog::sink_ptr inner_sink)
        : engine_(std::move(engine)), inner_sink_(std::move(inner_sink)) {
        if (!engine_) {
            throw spdlog::spdlog_ex("SpdlogMaskingSink: engine cannot be null");
        }
        if (!inner_sink_) {
            throw spdlog::spdlog_ex("SpdlogMaskingSink: inner_sink cannot be null");
        }
    }

    void log(const spdlog::details::log_msg &msg) override {
        std::string_view original_payload(msg.payload.data(), msg.payload.size());
        auto masked_result = engine_->process(original_payload);

        spdlog::details::log_msg masked_msg(msg.time, msg.source, msg.logger_name, msg.level, "");
        
        if (std::holds_alternative<std::string>(masked_result)) {
            const auto& masked_str = std::get<std::string>(masked_result);
            masked_msg.payload = spdlog::string_view_t(masked_str.data(), masked_str.size());
            inner_sink_->log(masked_msg);
        } else {
            masked_msg.payload = msg.payload;
            inner_sink_->log(masked_msg);
        }
    }

    void flush() override {
        inner_sink_->flush();
    }

    void set_pattern(const std::string &pattern) override {
        inner_sink_->set_pattern(pattern);
    }

    void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override {
        inner_sink_->set_formatter(std::move(sink_formatter));
    }

private:
    std::shared_ptr<shield::MaskingEngine> engine_;
    spdlog::sink_ptr inner_sink_;
};

using SpdlogMaskingSink_mt = SpdlogMaskingSink;
using SpdlogMaskingSink_st = SpdlogMaskingSink;

} // namespace integration
} // namespace shield
