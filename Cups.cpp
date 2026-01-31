#include "Cups.hpp"
#include <algorithm>
#include <cups/cups.h>
#include <cups/ipp.h>
#include <memory>
#include <span>

auto CupsPrinter::GetPrinters() -> std::expected<std::vector<CupsPrinter>, CupsError>
{
    std::vector<CupsPrinter> printers {};

    cups_dest_t*             ptr_tmp {};
    const int                NUM_DESTS = cupsGetDests(&ptr_tmp);

    // C-APIs returning negative integers for counts as error codes. Except sometimes when 0 is failure. I hate C-APIs
    // For some ridiculous reason Timeout (for example setting invalid env CUPS_SERVER=1.2.3.4:1234) shows up as 0
    // instead of negative. And also doesn't set cupsLastError().
    // Who designed this shit?
    if (NUM_DESTS < 0)
    {
        CupsError err{CupsError::EPhase::GET_DESTS};
        return std::unexpected<CupsError>{ std::in_place, std::move(err) };
    }
    if (NUM_DESTS == 0)
    {
        return {};
    }

    const auto DELETER = [NUM_DESTS](cups_dest_t* const ptr_p) { cupsFreeDests(NUM_DESTS, ptr_p); };
    const std::unique_ptr<cups_dest_t, decltype(DELETER)> UPTR_PRINTER_DESTS {std::exchange(ptr_tmp, nullptr), DELETER};

    const std::span<cups_dest_t> PRINTER_DESTS_SPAN {UPTR_PRINTER_DESTS.get(), static_cast<std::size_t>(NUM_DESTS)};
    printers.reserve(static_cast<std::size_t>(NUM_DESTS));

    for (const cups_dest_s& dest : PRINTER_DESTS_SPAN)
    {
        std::map<std::string, std::string, std::less<>> options {};
        const std::span<cups_option_s>     OPTIONS_SPAN {dest.options, static_cast<std::size_t>(dest.num_options)};
        for (const cups_option_s& option : OPTIONS_SPAN)
        {
            options.emplace(std::string {option.name}, std::string {option.value});
        }

        CupsPrinter printer {
          std::string {dest.name},
          std::string {dest.instance == nullptr ? "" : dest.instance},
          dest.is_default != 0, // because of course it is an integer. I hate C-APIs.
          std::move(options)
        };

        printers.push_back(std::move(printer));
    }

    return printers;
}

auto CupsPrinter::GetDefaultPrinter() -> std::expected<CupsPrinter, CupsError>
{
    // Cups' cupsGetDest(nullptr, nullptr, NUM_DESTS, DESTS.get()) requires all destinations anyway. It's a lot of
    // repeated code, so I will not refactor this, even if it removes a bunch of string operations.
    const auto PRINTERS = CupsPrinter::GetPrinters();
    if (PRINTERS.has_value())
    {
        auto defaultPrinter = std::ranges::find_if(*PRINTERS, [](const CupsPrinter& printer) { return printer.isDefault(); });
        if (defaultPrinter == PRINTERS->end())
        {
            CupsError err{CupsError::EPhase::NO_DEFAULT_PRINTER};
            return std::unexpected<CupsError> {std::in_place, std::move(err)};
        }
        return *defaultPrinter;
    }
    return std::unexpected<CupsError> {std::in_place, PRINTERS.error()};
}

auto CupsPrinter::buildName() const -> std::string
{
    return m_instance.empty() ? m_name : m_name + "/" + m_instance;
}

auto CupsPrinter::print(const std::string& jobName, const std::string& data, const EFormat FORMAT) const -> std::expected<void, CupsError>
{
    const auto                     DEST_NAME    = buildName();
    const char* const              ptr_destName = DEST_NAME.c_str();

    std::vector<cups_option_t>     opts {};
    opts.reserve(m_options.size());

    for (const auto& [key, value] : m_options)
    {
        // need non-const char* in opts, which is nonsense.
        // Apparently the cups library doesn't change the strings. Let's hope it's true.
        // Also apparently cups does not retain the pointers beyond cupsCreateJob call, so it's fine, I guess. Let's
        // hope. Bloody hell, why don't they declare it const if they don't change it?! I hate C-APIs :(

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast) // please kill me. I hate C-APIs.
        opts.emplace_back(const_cast<char*>(key.c_str()), const_cast<char*>(value.c_str()));
    }

    const int JOB_ID =
      cupsCreateJob(CUPS_HTTP_DEFAULT, ptr_destName, jobName.c_str(), static_cast<int>(opts.size()), opts.data());
    if (JOB_ID < 0)
    {
        CupsError err{CupsError::EPhase::CREATE_JOB};
        return std::unexpected<CupsError>{ std::in_place, std::move(err) };
    }

    const char* const ptr_formatString = [&FORMAT]()
    {
        switch (FORMAT)
        {
            case EFormat::TEXT:       return CUPS_FORMAT_TEXT;
            case EFormat::PDF:        return CUPS_FORMAT_PDF;
            case EFormat::POSTSCRIPT: return CUPS_FORMAT_POSTSCRIPT;
        }
        std::unreachable();
    }();

    const auto JOB_DELETER = [ptr_destName](const int* const ptr_jobId)
    {
        if (ptr_jobId)
        {
            cupsCancelJob(ptr_destName, *ptr_jobId);
        }
        delete ptr_jobId;
    };

    // cancels job on destruction, unless .dismiss() has been called.
    std::unique_ptr<int, decltype(JOB_DELETER)> jobGuard {new int(JOB_ID), JOB_DELETER};

    static constexpr bool IS_LAST_DOCUMENT_IN_JOB = true; // last_document being an int, like some kind of id. Great name, too. I hate C-APIs.
    if (cupsStartDocument(CUPS_HTTP_DEFAULT, ptr_destName, JOB_ID, "doc", ptr_formatString, static_cast<int>(IS_LAST_DOCUMENT_IN_JOB)) != HTTP_CONTINUE)
    {
        CupsError err{CupsError::EPhase::START_DOCUMENT};
        return std::unexpected<CupsError>{ std::in_place, std::move(err) };
    }
    if (cupsWriteRequestData(CUPS_HTTP_DEFAULT, data.data(), data.length()) != HTTP_CONTINUE)
    {
        CupsError err{CupsError::EPhase::WRITE_DATA};
        return std::unexpected<CupsError>{ std::in_place, std::move(err) };
    }
    if (cupsFinishDocument(CUPS_HTTP_DEFAULT, ptr_destName) != IPP_STATUS_OK)
    {
        CupsError err{CupsError::EPhase::FINISH_DOCUMENT};
        return std::unexpected<CupsError>{ std::in_place, std::move(err) };
    }

    delete jobGuard.release();
    return {};
}
