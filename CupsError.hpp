#include <cstdint>
#include <cups/cups.h>
#include <cups/ipp.h>
#include <string>

class CupsPrinter;

class CupsError
{
  public:
    enum class EPhase : std::uint8_t
    {
        GET_DESTS,
        NO_DEFAULT_PRINTER,
        CREATE_JOB,
        START_DOCUMENT,
        WRITE_DATA,
        FINISH_DOCUMENT
    };

  private:
    ipp_status_t m_status;
    EPhase       m_phase;
    std::string  m_message;

    friend CupsPrinter;

    // Get global state error. I hate C-APIs
    explicit CupsError(const EPhase PHASE)
            : m_status {PHASE == EPhase::NO_DEFAULT_PRINTER ? IPP_STATUS_OK : cupsLastError()},
              m_phase {PHASE},
              m_message {PHASE == EPhase::NO_DEFAULT_PRINTER ? "No default printer configured" : cupsLastErrorString()}
    {
        // Empty
    }

  public:

    [[nodiscard]]
    auto status() const noexcept -> ipp_status_t
    {
        return m_status;
    }

    [[nodiscard]]
    auto phase() const noexcept -> EPhase
    {
        return m_phase;
    }

    [[nodiscard]]
    auto message() const noexcept -> std::string_view
    {
        return m_message;
    }
};
