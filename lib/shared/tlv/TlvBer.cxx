
#include <algorithm>
#include <bit>
#include <iterator>
#include <optional>
#include <utility>

#include "TlvBer.hxx"

using namespace encoding;

TlvBer::TlvBer(TagClass tagClass, TagType tagType, const std::vector<uint8_t>& tagNumber, const std::vector<uint8_t>& tagComplete, const std::vector<uint8_t>& value) :
    m_tagClass(tagClass),
    m_tagType(tagType),
    m_tagNumber(tagNumber),
    m_tagComplete(tagComplete),
    m_value(value)
{
    ::Tlv::Tag = m_tagComplete;
    ::Tlv::Value = m_value;
}

TlvBer::TlvBer(TagClass tagClass, TagType tagType, const std::vector<uint8_t>& tagNumber, const std::vector<uint8_t>& tagComplete, std::vector<TlvBer>& values) :
    m_tagClass(tagClass),
    m_tagType(tagType),
    m_tagNumber(tagNumber),
    m_tagComplete(tagComplete),
    m_valuesConstructed(std::move(values))
{
    ::Tlv::Tag = m_tagComplete;
    ::Tlv::Value = std::span<const uint8_t>{};
}

/* static */
TlvBer::TagType
TlvBer::GetTagType(uint8_t tag)
{
    if ((tag & BitmaskType) == TypeConstructed) {
        return TagType::Constructed;
    } else {
        return TagType::Primitive;
    }
}

/* static */
TlvBer::TagClass
TlvBer::GetTagClass(uint8_t tag)
{
    switch (tag & BitmaskClass) {
    case ClassUniversal:
        return TagClass::Universal;
    case ClassApplication:
        return TagClass::Application;
    case ClassContextSpecific:
        return TagClass::ContextSpecific;
    case ClassPrivate:
    default:
        return TagClass::Private;
    }
}

Tlv::ParseResult
TlvBer::ParseTag(TagClass& tagClass, TagType& tagType,std::vector<uint8_t>& tagNumber, std::vector<uint8_t>& tagComplete, uint8_t tag){
    size_t bytesParsed;
    uint8_t tagArray[] = {tag};
    return ParseTag(tagClass,tagType,tagNumber,tagComplete,tagArray,bytesParsed);
}

template<typename Iterable>
Tlv::ParseResult
TlvBer::ParseTag(TagClass& tagClass, TagType& tagType,std::vector<uint8_t>& tagNumber, std::vector<uint8_t>& tagComplete, Iterable& data, size_t& bytesParsed){
    auto dataIt = std::cbegin(data);
    auto dataEnd = std::cend(data);
    if(dataIt==dataEnd) return Tlv::ParseResult::Failed;

    tagClass = GetTagClass(*dataIt);
    tagType = GetTagType(*dataIt);

    // Is tag short type?
    if ((*dataIt & BitmaskTagFirstByte) != TagValueLongField) {
        tagNumber.push_back(*dataIt & BitmaskTagShort);
        return Tlv::ParseResult::Succeeded;
    } else {
        // Tag is long-type. 
        int i = 0;
        do {
            i++;
            if (i>3) return Tlv::ParseResult::Failed;
            std::advance(dataIt,1);
            if(dataIt==dataEnd) return Tlv::ParseResult::Failed;
            tagNumber.push_back(*dataIt & BitmaskTagLong);
        } while ((*dataIt & BitmaskTagLastByte) != TagValueLastByte);
    }
    std::advance(dataIt,1); // advance the dataIt to once past the tag
    bytesParsed = std::distance(std::cbegin(data),dataIt);
    return Tlv::ParseResult::Succeeded;
}

template <typename Iterable>
Tlv::ParseResult
TlvBer::ParseLength(size_t& length, Iterable& data, size_t& bytesParsed){
    auto dataIt = std::cbegin(data);
    auto dataEnd = std::cend(data);
    if(dataIt==dataEnd) return Tlv::ParseResult::Failed;

    // Is length short form?
    if ((*dataIt & BitmaskLengthForm) == LengthFormShort) {
        length = *dataIt & BitmaskLengthShort;
    } else {
        // Length is long form.
        auto numOctets = *dataIt & BitmaskLengthNumOctets;
        if(numOctets > MaxNumOctetsInLengthEncoding) return Tlv::ParseResult::Failed;

        for (auto i = 0; i < numOctets; i++) {
            std::advance(dataIt,1);
            if(dataIt==dataEnd) return Tlv::ParseResult::Failed;

            length  = length << 8U;
            length |= *dataIt;
        }
    }
    std::advance(dataIt,1); // advance the dataIt to once past the lengthEncoding
    bytesParsed = std::distance(std::cbegin(data),dataIt);
    return Tlv::ParseResult::Succeeded;
}

template <typename Iterable>
Tlv::ParseResult
TlvBer::ParseValue(std::vector<uint8_t>& valueOutput,size_t length, Iterable& data, size_t& bytesParsed){
    auto dataIt = std::cbegin(data);
    auto dataEnd = std::cend(data);
    if(std::distance(dataIt,dataEnd) < length) return Tlv::ParseResult::Failed;
    valueOutput = std::vector<uint8_t> { dataIt, std::next(dataIt, length) };
    bytesParsed = length;
    return Tlv::ParseResult::Succeeded;
}

/* static */
template<typename Iterable>
Tlv::ParseResult
TlvBer::Parse(TlvBer **tlvOutput, Iterable& dataInput)
{
    if (!tlvOutput) {
        return Tlv::ParseResult::Failed;
    }

    std::span<uint8_t> data {std::cbegin(dataInput),std::cend(dataInput)};

    *tlvOutput = nullptr;
    auto parseResult = Tlv::ParseResult::Failed;

    // Parse tag.
    TagClass tagClass;
    TagType tagType;
    std::vector<uint8_t> tagNumber;
    std::vector<uint8_t> tagComplete;
    size_t offset = 0;
    size_t bytesParsed = 0;
    parseResult = ParseTag(tagClass,tagType,tagNumber,tagComplete,data,bytesParsed);
    if(Tlv::ParseResult::Succeeded != parseResult) return parseResult;

    // Parse length.
    offset += bytesParsed;
    std::size_t length = 0;
    parseResult = ParseLength(length,data.subspan(offset),bytesParsed);
    if(Tlv::ParseResult::Succeeded != parseResult) return parseResult;

    // Parse value.
    offset += bytesParsed;
    std::vector<uint8_t> value;
    parseResult = ParseValue(value,length,data.subspan(offset),bytesParsed);
    if(Tlv::ParseResult::Succeeded != parseResult) return parseResult;

    switch (tagType) {
    case TagType::Constructed:
        // TODO: parse nested tlvs
        break;
    case TagType::Primitive:
        *tlvOutput = std::make_unique<TlvBer>(tagClass, tagType, tagNumber, tagComplete, value).release();
        parseResult = ParseResult::Succeeded;
        break;
    }

    return parseResult;
}

std::span<const uint8_t>
TlvBer::GetTagComplete() const noexcept
{
    return m_tagComplete;
}

std::vector<TlvBer>
TlvBer::GetValues() const noexcept
{
    return m_valuesConstructed;
}

bool
TlvBer::IsConstructed() const noexcept
{
    return m_tagType == TlvBer::TagType::Constructed;
}

bool
TlvBer::IsPrimitive() const noexcept
{
    return m_tagType == TlvBer::TagType::Primitive;
}

void 
TlvBer::Builder::WriteData(uint32_t data)
{
    const auto b0 = static_cast<uint8_t>((data & 0xFF000000U) >> 24U);
    const auto b1 = static_cast<uint8_t>((data & 0x00FF0000U) >> 16U);
    const auto b2 = static_cast<uint8_t>((data & 0x0000FF00U) >>  8U);
    const auto b3 = static_cast<uint8_t>((data & 0x000000FFU) >>  0U);

    if constexpr (std::endian::native == std::endian::little) {
        m_data.push_back(b3);
        m_data.push_back(b2);
        m_data.push_back(b1);
        m_data.push_back(b0);
    } else {
        m_data.push_back(b0);
        m_data.push_back(b1);
        m_data.push_back(b2);
        m_data.push_back(b3);
    }
}

namespace detail
{
const std::array<std::tuple<uint8_t, std::size_t, std::optional<uint8_t>>, 5> LengthInfo = {
    std::make_tuple(1, 0x0000007FU, std::nullopt),
    std::make_tuple(2, 0x000000FFU, TlvBer::LengthTag2Byte),
    std::make_tuple(3, 0x0000FFFFU, TlvBer::LengthTag3Byte),
    std::make_tuple(4, 0x00FFFFFFU, TlvBer::LengthTag4Byte),
    std::make_tuple(5, 0xFFFFFFFFU, TlvBer::LengthTag5Byte)
};
} // namespace detail

/* static */
std::vector<uint8_t>
TlvBer::GetLengthEncoding(std::size_t length)
{
    for (const auto& [numBytes, maxValue, numOctetsInLengthFieldValue]: detail::LengthInfo) {
        if (length <= maxValue) {
            // Short-form, values 0-127.
            if (!numOctetsInLengthFieldValue.has_value()) {
                return { static_cast<uint8_t>(length) };
            }

            // Long-form, values 128+.
            std::vector<uint8_t> encoding(numBytes);
            for (auto i = numBytes; numBytes > 0; i--) {
                encoding[i] = static_cast<uint8_t>(length & 0xFFU);
                length >>= 8U;
            }
            encoding[0] = numOctetsInLengthFieldValue.value();
            return encoding;
        }
    }

    throw std::length_error("length out of range");        
}

void
TlvBer::Builder::WriteData(std::span<const uint8_t> data)
{
    m_data.insert(std::cend(m_data), std::cbegin(data), std::cend(data));
}

void
TlvBer::Builder::WriteValue(uint8_t value)
{
    m_data.push_back(1);
    m_data.push_back(value);
}

void
TlvBer::Builder::WriteValue(std::span<const uint8_t> value)
{
    WriteData(value.size_bytes());
    WriteData(value);
}

TlvBer::Builder&
TlvBer::Builder::SetTag(uint8_t tag)
{
    TlvBer::ParseTag(m_tagClass,m_tagType,m_tagNumber,m_tagComplete,tag);
    return *this;
}

TlvBer::Builder&
TlvBer::Builder::AddTlv(auto&& func)
{
    func();
    m_validateConstructed = true;
    return *this;
}

TlvBer::Builder&
TlvBer::Builder::AddTlv(const Tlv& tlv)
{
    return AddTlv([&]{
        WriteData(tlv.Tag);
        WriteValue(tlv.Value);
    });
}

TlvBer 
TlvBer::Builder::Build()
{
    ValidateTag();
    return TlvBer{ m_tagClass, m_tagType, m_tagNumber, m_tagComplete, m_data };
}

void
TlvBer::Builder::ValidateTag()
{
    if (m_validateConstructed && m_tagType != TagType::Constructed) {
        throw InvalidTlvBerTagException();
    }
}
