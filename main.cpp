#include "Cups.hpp"
#include <print>
#include <utility>

auto main() -> int
{
    auto printers = CupsPrinter::GetPrinters();
    if (!printers)
    {
        std::println("Error during phase {}: {}", std::to_underlying(printers.error().phase()), printers.error().message());
        return 1;
    }

    for (const auto& printer : printers.value())
    {
        std::println(
          "Printer: {}{}\n  {}\n", printer.getName(), printer.isDefault() ? " (default)" : "", printer.getOptions()
        );
    }


    auto printer = CupsPrinter::GetDefaultPrinter();
    if (!printer)
    {
        std::println("Error during phase {}: {}", std::to_underlying(printer.error().phase()), printer.error().message());
        return 1;
    }

    const auto& defaultPrinter = *printer;
    std::println("Default printer: {}", defaultPrinter.getName());

    const std::expected<void, CupsError> STATUS =
      defaultPrinter.print("test", "This was printed from my c++ program.", CupsPrinter::EFormat::TEXT);
    // const std::expected<void, CupsError> STATUS = {};

    if (!STATUS)
    {
        std::println("Job was not queued.");
        std::println("Error during phase {}: {}", std::to_underlying(STATUS.error().phase()), STATUS.error().message());
        return 2;
    }

    std::println("Printing queued successfully.");

    return 0;
}
