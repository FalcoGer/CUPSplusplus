#include "Cups.hpp"
#include <print>
#include <utility>

auto main() -> int
{
    if (!CupsPrinter::GetPrinters()
           .and_then(
             [](const auto& printers) -> std::expected<std::vector<CupsPrinter>, CupsError>
             {
                 for (const auto& printer : printers)
                 {
                     std::println(
                       "Printer: {}{}\n  {}\n",
                       printer.getName(),
                       printer.isDefault() ? " (default)" : "",
                       printer.getOptions()
                     );
                 }
                 return printers;
             }
           )
           .transform_error(
             [](const auto& error)
             {
                 std::println("Error during phase {}: {}", std::to_underlying(error.phase()), error.message());
                 return error;
             }
           ))
    {
        return 1;
    }

    if (!CupsPrinter::GetDefaultPrinter()
           .and_then(
             [](const auto& printer)
             {
                 return printer.print("test", "This was printed from my c++ program.", CupsPrinter::EFormat::TEXT);
                 // return std::expected<void, CupsError>{};
             }
           )
           .and_then(
             []() -> std::expected<void, CupsError>
             {
                 std::println("Printing queued successfully.");
                 return std::expected<void, CupsError> {};
             }
           )
           .transform_error(
             [](const auto& error)
             {
                 std::println("Error during phase {}: {}", std::to_underlying(error.phase()), error.message());
                 return error;
             }
           ))
    {
        return 2;
    }

    return 0;
}
