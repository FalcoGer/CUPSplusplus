#pragma once

#include "CupsError.hpp"
#include <cstdint>
#include <cups/cups.h>
#include <expected>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// TODO: use deducing this to improve performance
// TODO: stop shoving strings around when not required to improve performance.
class CupsPrinter
{
  private:
    std::string m_name;
    std::string m_instance;
    bool         m_isDefault;
    std::map<std::string, std::string, std::less<>> m_options;

    CupsPrinter(std::string name, std::string instance, bool isDefault, std::map<std::string, std::string, std::less<>> options)
            : m_name(std::move(name)),
              m_instance(std::move(instance)),
              m_isDefault(isDefault),
              m_options(std::move(options))
    {}

    [[nodiscard]]
    auto buildName() const -> std::string;

    class JobGuard
    {
      private:
        int         m_jobId;
        std::string m_name;
        bool        m_isArmed;

      public:
        JobGuard()                       = delete;
        JobGuard(const JobGuard&)        = delete;
        JobGuard(JobGuard&& other) noexcept
                : m_jobId {other.m_jobId}, m_name {std::move(other.m_name)}, m_isArmed {std::exchange(other.m_isArmed, false)}
        {
            // empty
        }

        void operator= (const JobGuard&) = delete;
        auto operator= (JobGuard&& other) noexcept -> JobGuard&
        {
            if (&other == this)
            {
                return *this;
            }
            m_jobId = other.m_jobId;
            m_name  = std::move(other.m_name);
            m_isArmed = std::exchange(other.m_isArmed, false);
            return *this;
        }
        JobGuard(int jobId, std::string_view name) : m_jobId {jobId}, m_name {name}, m_isArmed {true}
        {
            // empty
        }
        ~JobGuard()
        {
            if (m_jobId > 0 && m_isArmed)
            {
                cupsCancelJob(m_name.c_str(), m_jobId);
            }
        }
        void dismiss() { m_isArmed = false; }
    };

  public:
    enum class EFormat : std::uint8_t
    {
        TEXT,
        PDF,
        POSTSCRIPT
       // TODO: are there others?
    };

    struct NoPrinters {};

    static auto GetPrinters() -> std::expected<std::vector<CupsPrinter>, CupsError>;
    static auto GetDefaultPrinter() -> std::expected<CupsPrinter, CupsError>;

    [[nodiscard]]
    auto getName() const noexcept -> const std::string&
    {
        return m_name;
    }
    [[nodiscard]]
    auto isDefault() const -> bool
    {
        return m_isDefault;
    }

    [[nodiscard]]
    auto getOptions() const -> const std::map<std::string, std::string, std::less<>>&
    {
        return m_options;
    }

    [[nodiscard]]
    auto getInstance() const -> const std::string&
    {
        return m_instance;
    }

    void setOption(const std::string& key, std::string value)
    {
        m_options.insert_or_assign(key, std::move(value));
    }

    [[nodiscard]]
    auto print(const std::string& jobName, const std::string& data, EFormat format) const -> std::expected<void, CupsError>;
};
