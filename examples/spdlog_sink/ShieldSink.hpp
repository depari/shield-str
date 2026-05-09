#pragma once
// examples/spdlog_sink/ShieldSink.hpp — spdlog custom sink with shield-str masking

#include <shield/shield.hpp>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/log_msg.h>
#include <mutex>
#include <memory>

namespace shield {

/**
 * @brief spdlog sink that masks sensitive data before passing to downstream.
 *
 * Usage:
 * @code
 *   auto mgr    = shield::RuleManager::from_file("rules/default_rules.json");
 *   auto engine = std::make_shared<shield::MaskingEngine>(mgr);
 *
 *   auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("app.log");
 *   auto shield_sink = std::make_shared<shield::ShieldSink<std::mutex>>(engine, file_sink);
 *
 *   auto logger = std::make_shared<spdlog::logger>("secure", shield_sink);
 *   spdlog::register_logger(logger);
 * @endcode
 */
template <typename Mutex>
class ShieldSink : public spdlog::sinks::base_sink<Mutex> {
public:
    ShieldSink(
        std::shared_ptr<shield::MaskingEngine> engine,
        spdlog::sink_ptr                       downstream)
        : engine_(std::move(engine))
        , downstream_(std::move(downstream))
    {}

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        // Format the message first
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        std::string_view raw(formatted.data(), formatted.size());

        // Apply masking
        auto result = engine_->process(raw);

        // Build a modified log_msg with masked payload if needed
        if (std::holds_alternative<std::string>(result)) {
            const auto& masked = std::get<std::string>(result);
            // Re-emit as raw string to downstream
            spdlog::details::log_msg masked_msg{
                msg.source, msg.level,
                spdlog::string_view_t(masked.data(), masked.size())
            };
            // Write the already-formatted masked string directly
            spdlog::memory_buf_t masked_buf;
            masked_buf.append(masked.data(), masked.data() + masked.size());
            downstream_->log(masked_msg);
        } else {
            downstream_->log(msg);
        }
    }

    void flush_() override {
        downstream_->flush();
    }

private:
    std::shared_ptr<shield::MaskingEngine> engine_;
    spdlog::sink_ptr                       downstream_;
};

using ShieldSink_mt = ShieldSink<std::mutex>;
using ShieldSink_st = ShieldSink<spdlog::details::null_mutex>;

} // namespace shield
