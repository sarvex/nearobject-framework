
#include "TlvSimple.hxx"

using namespace encoding;

/* static */
Tlv::ParseResult
TlvSimple::Parse(std::unique_ptr<Tlv>* tlvOutput, gsl::span<std::byte> /* data */)
{
    std::unique_ptr<Tlv> tlvParsed{ nullptr };
    Tlv::ParseResult parseResult = Tlv::ParseResult::Failed;

    if (parseResult == Tlv::ParseResult::Succeeded) {
        *tlvOutput = std::move(tlvParsed);
    }

    return parseResult;
}