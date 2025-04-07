/*
* Copyright 2024 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#ifdef ZXING_EXPERIMENTAL_API

#include "WriteBarcode.h"
#include "BitMatrix.h"

#if !defined(ZXING_READERS) && !defined(ZXING_WRITERS)
#include "Version.h"
#endif

#include <sstream>

#ifdef ZXING_USE_ZINT

#include "ZXCType.h"
#include <zint.h>

#else

struct zint_symbol {};

#endif // ZXING_USE_ZINT

namespace ZXing {

struct CreatorOptions::Data
{
	BarcodeFormat format;
	bool readerInit = false;
	bool forceSquareDataMatrix = false;
	std::string ecLevel;
#ifdef ZXING_USE_ZINT
	bool withQuietZones = true;
	bool stacked = false; // DataBar
	int margin = 0;
	CharacterSet encoding = CharacterSet::Unknown;
	int rotate = 0;
	ECI eci = ECI::Unknown;
	int vers = 0;
	int mask = -1;
	float height = 0.0f;
	bool gs1 = false;
	bool debug = false;
#endif

	// symbol size (qrcode, datamatrix, etc), map from I, 'WxH'
	// structured_append (idx, cnt, ID)

	mutable unique_zint_symbol zint;

#ifndef __cpp_aggregate_paren_init
	Data(BarcodeFormat f) : format(f) {}
#endif
};

#define ZX_PROPERTY(TYPE, NAME) \
	TYPE CreatorOptions::NAME() const noexcept { return d->NAME; } \
	CreatorOptions& CreatorOptions::NAME(TYPE v)& { return d->NAME = std::move(v), *this; } \
	CreatorOptions&& CreatorOptions::NAME(TYPE v)&& { return d->NAME = std::move(v), std::move(*this); }

	ZX_PROPERTY(BarcodeFormat, format)
	ZX_PROPERTY(bool, readerInit)
	ZX_PROPERTY(bool, forceSquareDataMatrix)
	ZX_PROPERTY(std::string, ecLevel)
#ifdef ZXING_USE_ZINT
	ZX_PROPERTY(bool, withQuietZones)
	ZX_PROPERTY(bool, stacked)
	ZX_PROPERTY(int, margin)
	ZX_PROPERTY(CharacterSet, encoding)
	ZX_PROPERTY(int, rotate)
	ZX_PROPERTY(ECI, eci)
	ZX_PROPERTY(int, vers)
	ZX_PROPERTY(int, mask)
	ZX_PROPERTY(float, height)
	ZX_PROPERTY(bool, gs1)
	ZX_PROPERTY(bool, debug)
#endif

#undef ZX_PROPERTY

CreatorOptions::CreatorOptions(BarcodeFormat format) : d(std::make_unique<Data>(format)) {}
CreatorOptions::~CreatorOptions() = default;
CreatorOptions::CreatorOptions(CreatorOptions&&) = default;
CreatorOptions& CreatorOptions::operator=(CreatorOptions&&) = default;


struct WriterOptions::Data
{
	int scale = 0;
	int sizeHint = 0;
	int rotate = 0;
	bool withHRT = false;
	bool withQuietZones = true;
};

#define ZX_PROPERTY(TYPE, NAME) \
	TYPE WriterOptions::NAME() const noexcept { return d->NAME; } \
	WriterOptions& WriterOptions::NAME(TYPE v)& { return d->NAME = std::move(v), *this; } \
	WriterOptions&& WriterOptions::NAME(TYPE v)&& { return d->NAME = std::move(v), std::move(*this); }

ZX_PROPERTY(int, scale)
ZX_PROPERTY(int, sizeHint)
ZX_PROPERTY(int, rotate)
ZX_PROPERTY(bool, withHRT)
ZX_PROPERTY(bool, withQuietZones)

#undef ZX_PROPERTY

WriterOptions::WriterOptions() : d(std::make_unique<Data>()) {}
WriterOptions::~WriterOptions() = default;
WriterOptions::WriterOptions(WriterOptions&&) = default;
WriterOptions& WriterOptions::operator=(WriterOptions&&) = default;

static bool IsLinearCode(BarcodeFormat format)
{
	return BarcodeFormats(BarcodeFormat::LinearCodes).testFlag(format);
}

#ifdef ZXING_USE_ZINT
static bool SupportsGS1(BarcodeFormat format)
{
	return BarcodeFormats(BarcodeFormat::Aztec | BarcodeFormat::Code128 | BarcodeFormat::Code16K | BarcodeFormat::DataMatrix
						  | BarcodeFormat::DotCode | BarcodeFormat::QRCode | BarcodeFormat::RMQRCode).testFlag(format);
}

static bool IsEANUPC(BarcodeFormat format)
{
	return BarcodeFormats(BarcodeFormat::EAN13 | BarcodeFormat::EAN8 | BarcodeFormat::UPCA | BarcodeFormat::UPCE
						  | BarcodeFormat::RMQRCode).testFlag(format);
}

static int EANUPCLength(BarcodeFormat format)
{
	return format == BarcodeFormat::EAN13 ? 13 : format == BarcodeFormat::EAN8 ? 8 :
			format == BarcodeFormat::UPCA ? 12 : format == BarcodeFormat::UPCE ? 10 : 0;
}
#endif

static std::string ToSVG(ImageView iv)
{
	if (!iv.data())
		return {};

	// see https://stackoverflow.com/questions/10789059/create-qr-code-in-vector-image/60638350#60638350

	std::ostringstream res;

	res << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		<< "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" viewBox=\"0 0 " << iv.width() << " " << iv.height()
		<< "\" stroke=\"none\">\n"
		<< "<path d=\"";

	for (int y = 0; y < iv.height(); ++y)
		for (int x = 0; x < iv.width(); ++x)
			if (*iv.data(x, y) == 0)
				res << "M" << x << "," << y << "h1v1h-1z";

	res << "\"/>\n</svg>";

	return res.str();
}

static Image ToImage(BitMatrix bits, bool isLinearCode, const WriterOptions& opts)
{
	bits.flipAll();
	auto symbol = Inflate(std::move(bits), opts.sizeHint(),
						  isLinearCode ? std::clamp(opts.sizeHint() / 2, 50, 300) : opts.sizeHint(),
						  opts.withQuietZones() ? 10 : 0);
	auto bitmap = ToMatrix<uint8_t>(symbol);
	auto iv = Image(symbol.width(), symbol.height());
	std::memcpy(const_cast<uint8_t*>(iv.data()), bitmap.data(), iv.width() * iv.height());
	return iv;
}

} // namespace ZXing


#ifdef ZXING_WRITERS

#ifdef ZXING_USE_ZINT
#include "ECI.h"

#include <charconv>
#include <zint.h>

namespace ZXing {

struct BarcodeFormatZXing2Zint
{
	BarcodeFormat zxing;
	int zint;
};

static constexpr BarcodeFormatZXing2Zint barcodeFormatZXing2Zint[] = {
	{BarcodeFormat::Aztec, BARCODE_AZTEC},
	{BarcodeFormat::Codabar, BARCODE_CODABAR},
	{BarcodeFormat::CodablockF, BARCODE_CODABLOCKF},
	{BarcodeFormat::Code128, BARCODE_CODE128},
	{BarcodeFormat::Code16K, BARCODE_CODE16K},
	{BarcodeFormat::Code39, BARCODE_EXCODE39},
	{BarcodeFormat::Code93, BARCODE_CODE93},
	{BarcodeFormat::DataBar, BARCODE_DBAR_OMN},
	{BarcodeFormat::DataBarExpanded, BARCODE_DBAR_EXP},
	{BarcodeFormat::DataBarLimited, BARCODE_DBAR_LTD},
	{BarcodeFormat::DataMatrix, BARCODE_DATAMATRIX},
	{BarcodeFormat::DotCode, BARCODE_DOTCODE},
	{BarcodeFormat::DXFilmEdge, BARCODE_DXFILMEDGE},
	{BarcodeFormat::EAN8, BARCODE_EANX},
	{BarcodeFormat::EAN13, BARCODE_EANX},
	{BarcodeFormat::HanXin, BARCODE_HANXIN},
	{BarcodeFormat::ITF, BARCODE_C25INTER},
	{BarcodeFormat::MaxiCode, BARCODE_MAXICODE},
	{BarcodeFormat::MicroPDF417, BARCODE_MICROPDF417},
	{BarcodeFormat::MicroQRCode, BARCODE_MICROQR},
	{BarcodeFormat::PDF417, BARCODE_PDF417},
	{BarcodeFormat::QRCode, BARCODE_QRCODE},
	{BarcodeFormat::RMQRCode, BARCODE_RMQR},
	{BarcodeFormat::UPCA, BARCODE_UPCA},
	{BarcodeFormat::UPCE, BARCODE_UPCE},
};

static int ParseECLevel(int symbology, std::string_view s)
{
	constexpr std::string_view EC_LABELS_QR[4] = {"L", "M", "Q", "H"};
	int res = 0;

	// Convert L/M/Q/H to Zint 1-4
	if (Contains({BARCODE_QRCODE, BARCODE_MICROQR, BARCODE_RMQR}, symbology))
		if ((res = IndexOf(EC_LABELS_QR, s)) != -1)
			return res + 1;

	// Convert as number into `res`, up to optional percent
	if (std::from_chars(s.data(), s.data() + s.size() - (s.back() == '%'), res).ec != std::errc{})
		throw std::invalid_argument("Invalid ecLevel: '" + std::string(s) + "'");

	auto findClosestECLevel = [](const std::vector<int>& list, int val) {
		int mIdx = -2, mAbs = 100;
		for (int i = 0; i < Size(list); ++i)
			if (int abs = std::abs(val - list[i]); abs < mAbs) {
				mIdx = i;
				mAbs = abs;
			}
		return mIdx + 1;
	};

	// Convert percentage to Zint
	if (s.back() == '%') {
		switch (symbology) {
		case BARCODE_QRCODE: return findClosestECLevel({20, 37, 55, 65}, res);
		case BARCODE_MICROQR: return findClosestECLevel({20, 37, 55}, res);
		case BARCODE_RMQR: return res <= 46 ? 2 : 4;
		case BARCODE_AZTEC: return findClosestECLevel({10, 23, 36, 50}, res);
		case BARCODE_PDF417:
			// TODO: do something sensible with PDF417?
		case BARCODE_HANXIN: return findClosestECLevel({8, 15, 23, 30}, res);

		default:
			return -1;
		}
	}

	switch (symbology) {
	case BARCODE_AZTEC:
		if (res >= 1 && res <= 4)
			return res;
		break;
	case BARCODE_MICROQR:
		if (res >= 1 && res <= 3)
			return res;
		break;
	case BARCODE_PDF417:
		if (res >= 0 && res <= 8)
			return res;
		break;
	case BARCODE_QRCODE:
		if (res >= 1 && res <= 4)
			return res;
		break;
	case BARCODE_RMQR:
		if (res == 2 || res == 4)
			return res;
		break;
	case BARCODE_HANXIN:
		if (res >= 1 && res <= 4)
			return res;
		break;
	}

	return -1;
}

static constexpr struct { BarcodeFormat format; SymbologyIdentifier si; } barcodeFormat2SymbologyIdentifier[] = {
	{BarcodeFormat::Aztec, {'z', '0', 3}}, // '1' GS1, '2' AIM
	{BarcodeFormat::Codabar, {'F', '0'}}, // if checksum processing were implemented and checksum present and stripped then modifier would be 4
	{BarcodeFormat::CodablockF, {'O', '4'}}, // '5' GS1
	{BarcodeFormat::Code128, {'C', '0'}}, // '1' GS1, '2' AIM
	{BarcodeFormat::Code16K, {'K', '0'}}, // '1' GS1, '2' AIM, '4' D1 PAD
	{BarcodeFormat::Code39, {'A', '0'}}, // '3' checksum, '4' extended, '7' checksum,extended
	{BarcodeFormat::Code93, {'G', '0'}}, // no modifiers
	{BarcodeFormat::DataBar, {'e', '0'}}, // Note: not marked as GS1
	{BarcodeFormat::DataBarExpanded, {'e', '0', 0, AIFlag::GS1}},
	{BarcodeFormat::DataBarLimited, {'e', '0'}}, // Note: not marked as GS1
	{BarcodeFormat::DataMatrix, {'d', '1', 3}}, // '2' GS1, '3' AIM
	{BarcodeFormat::DotCode, {'J', '0', 3}}, // '1' GS1, '2' AIM
	{BarcodeFormat::DXFilmEdge, {}},
	{BarcodeFormat::EAN8, {'E', '4'}},
	{BarcodeFormat::EAN13, {'E', '0'}},
	{BarcodeFormat::HanXin, {'h', '0', 1}}, // '2' GS1
	{BarcodeFormat::ITF, {'I', '0'}}, // '1' check digit
	{BarcodeFormat::MaxiCode, {'U', '0', 2}}, // '1' mode 2 or 3
	{BarcodeFormat::MicroPDF417, {'L', '2', char(-1)}},
	{BarcodeFormat::MicroQRCode, {'Q', '1', 1}},
	{BarcodeFormat::PDF417, {'L', '2', char(-1)}},
	{BarcodeFormat::QRCode, {'Q', '1', 1}}, // '3' GS1, '5' AIM
	{BarcodeFormat::RMQRCode, {'Q', '1', 1}}, // '3' GS1, '5' AIM
	{BarcodeFormat::UPCA, {'E', '0'}},
	{BarcodeFormat::UPCE, {'E', '0'}},
};

static SymbologyIdentifier SymbologyIdentifierZint2ZXing(const CreatorOptions& opts, const ByteArray& ba)
{
	const BarcodeFormat format = opts.format();

	auto i = FindIf(barcodeFormat2SymbologyIdentifier, [format](auto& v) { return v.format == format; });
	assert(i != std::end(barcodeFormat2SymbologyIdentifier));
	SymbologyIdentifier ret = i->si;

	if (format == BarcodeFormat::EAN13 || format == BarcodeFormat::UPCA || format == BarcodeFormat::UPCE) {
		auto end = std::min(ba.begin() + EANUPCLength(format) + 1, ba.end());
		if (std::find(ba.begin(), end, ' ') != end) // Have EAN-2/5 add-on?
			ret.modifier = '3'; // Combined packet, EAN-13, UPC-A, UPC-E, with add-on
	} else if (format == BarcodeFormat::Code39) {
		if (FindIf(ba, zx_iscntrl) != ba.end()) // Extended Code 39?
			ret.modifier = static_cast<char>(ret.modifier + 4);
	} else if ((opts.gs1() && SupportsGS1(format)) || format == BarcodeFormat::DataBar || format == BarcodeFormat::DataBarLimited) {
		if (format == BarcodeFormat::Aztec || format == BarcodeFormat::Code128 || format == BarcodeFormat::Code16K
				|| format == BarcodeFormat::DotCode)
			ret.modifier = '1';
		else if (format == BarcodeFormat::DataMatrix)
			ret.modifier = '2';
		else if (format == BarcodeFormat::QRCode || format == BarcodeFormat::RMQRCode)
			ret.modifier = '3';
		ret.aiFlag = AIFlag::GS1;
	}

	return ret;
}

static std::string ECLevelZint2ZXing(const zint_symbol* zint)
{
	static const char EC_LABELS_QR[4][2] = {{"L"}, {"M"}, {"Q"}, {"H"}};

	const int symbology = zint->symbology;
	const int option_1 = zint->option_1;

	switch (symbology) {
	case BARCODE_AZTEC:
		if ((option_1 >> 8) >= 0 && (option_1 >> 8) <= 99)
			return std::to_string(option_1 >> 8) + "%";
		break;
	case BARCODE_MAXICODE:
		// Mode
		if (option_1 >= 2 && option_1 <= 6)
			return std::to_string(option_1);
		break;
	case BARCODE_MICROQR:
		// Convert to L/M/Q
		if (option_1 >= 1 && option_1 <= 3)
			return EC_LABELS_QR[option_1 - 1];
		break;
	case BARCODE_PDF417:
	case BARCODE_PDF417COMP:
		// Convert to percentage
		if (option_1 >= 0 && option_1 <= 8) {
			int overhead = symbology == BARCODE_PDF417COMP ? 35 : 69;
			int cols = (zint->width - overhead) / 17;
			int tot_cws = zint->rows * cols;
			assert(tot_cws);
			return std::to_string((2 << option_1) * 100 / tot_cws) + "%";
		}
		break;
	case BARCODE_MICROPDF417:
		if ((option_1 >> 8) >= 0 && (option_1 >> 8) <= 99)
			return std::to_string(option_1 >> 8) + "%";
		break;
	case BARCODE_QRCODE:
		// Convert to L/M/Q/H
		if (option_1 >= 1 && option_1 <= 4)
			return EC_LABELS_QR[option_1 - 1];
		break;
	case BARCODE_RMQR:
		// Convert to M/H
		if (option_1 == 2 || option_1 == 4)
			return EC_LABELS_QR[option_1 - 1];
		break;
	case BARCODE_HANXIN:
		if (option_1 >= 1 && option_1 <= 4)
			return "L" + std::to_string(option_1);
		break;
	default:
		break;
	}

	return {};
}

static Position PositionZint2ZXing(const CreatorOptions& opts, const BitMatrix& bits)
{
	const BarcodeFormat format = opts.format();
	const bool stacked = opts.stacked();
	const int rotate = opts.rotate();

	Position pos;
	int left, top, width, height;
	if (bits.findBoundingBox(left, top, width, height)) {

		// TODO: Hack
		if (format == BarcodeFormat::Aztec) {
			// Do nothing
		} else if (format == BarcodeFormat::DataBar) {
			if (stacked) {
				width--;
				left++;
			} else {
				// Leave width & left
			}
			height--; // Point at bottommost
		} else if (format == BarcodeFormat::DataBarExpanded) {
			if (stacked) {
			} else {
				width--; // Point at rightmost
				height--; // Point at bottommost
				left++;
				width -= 2;
			}
		} else {
			width--; // Point at rightmost
			height--; // Point at bottommost
		}

		if (rotate == 90)
			pos = Position(PointI{left + width, top}, PointI{left + width, top + height}, PointI{left, top + height}, PointI{left, top});
		else if (rotate == 180)
			pos = Position(PointI{left + width, top + height}, PointI{left, top + height}, PointI{left, top}, PointI{left + width, top});
		else if (rotate == 270)
			pos = Position(PointI{left, top + height}, PointI{left, top}, PointI{left + width, top}, PointI{left + width, top + height});
		else
			pos = Position(PointI{left, top}, PointI{left + width, top}, PointI{left + width, top + height}, PointI{left, top + height});
	}
	return pos;
}

static int VersionZint2ZXing(const zint_symbol* zint)
{
	const int symbology = zint->symbology;
	const int option_2 = zint->option_2;

	switch (symbology) {
	case BARCODE_AZTEC:
		// Convert to layers (can differentate between Compact and Full using `bits` dimensions)
		if (option_2 >= 1 && option_2 <= 36)
			return option_2 <= 4 ? option_2 : option_2 - 4;
		break;
	case BARCODE_DATAMATRIX:
		// Same as Zint
		if (option_2 >= 1 && option_2 <= 48)
			return option_2;
		break;
	case BARCODE_MICROQR:
		// Same as Zint
		if (option_2 >= 1 && option_2 <= 4)
			return option_2;
		break;
	case BARCODE_QRCODE:
		// Same as Zint
		if (option_2 >= 1 && option_2 <= 40)
			return option_2;
		break;
	case BARCODE_RMQR:
		// Same as Zint
		if (option_2 >= 1 && option_2 <= 32)
			return option_2;
		break;
	case BARCODE_HANXIN:
		// Same as Zint
		if (option_2 >= 1 && option_2 <= 84)
			return option_2;
		break;
	default:
		break;
	}
	return 0;
}

static int DataMaskZint2ZXing(const zint_symbol* zint)
{
	const int mask = (zint->option_3 >> 8) & 0x0F;

	switch (zint->symbology) {
	case BARCODE_MICROQR:
		if (mask >= 1 && mask <= 4)
			return mask - 1;
		break;
	case BARCODE_QRCODE:
		if (mask >= 1 && mask <= 8)
			return mask - 1;
		break;
	case BARCODE_RMQR:
		return 4; // Set to 4 (pattern 100)
	case BARCODE_DOTCODE:
		if (mask >= 1 && mask <= 8)
			return mask - 1;
		break;
	case BARCODE_HANXIN:
		if (mask >= 1 && mask <= 4)
			return mask - 1;
		break;
	default:
		break;
	}
	return -1;
}

zint_symbol* CreatorOptions::zint() const
{
	auto& zint = d->zint;

	if (!zint) {
#ifdef PRINT_DEBUG
		printf("zint version: %d, sizeof(zint_symbol): %ld\n", ZBarcode_Version(), sizeof(zint_symbol));
#endif
		zint.reset(ZBarcode_Create());

		auto i = FindIf(barcodeFormatZXing2Zint, [zxing = format()](auto& v) { return v.zxing == zxing; });
		if (i == std::end(barcodeFormatZXing2Zint))
			throw std::invalid_argument("unsupported barcode format: " + ToString(format()));

		if (format() == BarcodeFormat::Code128 && gs1())
			zint->symbology = BARCODE_GS1_128;
		else if (format() == BarcodeFormat::DataBar && stacked())
			zint->symbology = BARCODE_DBAR_OMNSTK;
		else if (format() == BarcodeFormat::DataBarExpanded && stacked())
			zint->symbology = BARCODE_DBAR_EXPSTK;
		else
			zint->symbology = i->zint;

		zint->scale = 0.5f;
	}

	return zint.get();
}

#define CHECK(ZINT_CALL) \
	if (int err = (ZINT_CALL); err >= ZINT_ERROR) \
		throw std::invalid_argument(std::string(zint->errtxt) + " (retval: " + std::to_string(err) + ")");

#define CHECK_NO_ERRTXT(ZINT_CALL) \
	if (int err = (ZINT_CALL); err >= ZINT_ERROR) \
		throw std::invalid_argument(std::to_string(err))

Barcode CreateBarcode(const void* data, int size, int mode, const CreatorOptions& opts)
{
	auto zint = opts.zint();
	const auto format = opts.format();

	zint->input_mode = mode == -1 ? opts.gs1() && SupportsGS1(format) ? GS1_MODE : UNICODE_MODE : mode;

	zint->show_hrt = 0;
	zint->output_options |= OUT_BUFFER_INTERMEDIATE | BARCODE_RAW_TEXT;

	zint->output_options |= opts.withQuietZones() ? BARCODE_QUIET_ZONES : BARCODE_NO_QUIET_ZONES;
	if (opts.height() && IsLinearCode(format))
		zint->height = opts.height();
	else
		zint->output_options |= COMPLIANT_HEIGHT;
	if (opts.readerInit())
		zint->output_options |= READER_INIT;

	zint->whitespace_width = zint->whitespace_height = opts.margin();

	if (!opts.ecLevel().empty() && !IsLinearCode(format))
		zint->option_1 = ParseECLevel(zint->symbology, opts.ecLevel());

	ECI eci = opts.eci() != ECI::Unknown ? opts.eci() : mode == DATA_MODE ? ECI::Binary : ECI::Unknown;
	if (eci != ECI::Unknown && ZBarcode_Cap(zint->symbology, ZINT_CAP_ECI))
		zint->eci = ToInt(eci);

	if (opts.vers() && !IsLinearCode(format))
		zint->option_2 = opts.vers();

	if (opts.mask() != -1 && (format == BarcodeFormat::QRCode || format == BarcodeFormat::MicroQRCode))
		zint->option_3 = (zint->option_3 & 0xFF) | ((opts.mask() + 1) << 8);

	if (opts.debug())
		zint->debug = 1;

	CHECK(ZBarcode_Encode_and_Buffer(zint, (const uint8_t*)data, size, opts.rotate()));

#ifdef PRINT_DEBUG
	printf("create symbol with size: %dx%d\n", zint->width, zint->rows);
#endif

	ByteArray ba;
	auto source = reinterpret_cast<const uint8_t *>(zint->raw_segs[0].source);
	ba.insert(ba.begin(), source, source + zint->raw_segs[0].length);

	if (IsEANUPC(format)) {
		// Replace Zint add-on indicator "+" with space
		auto end = std::min(ba.begin() + EANUPCLength(format) + 1, ba.end());
		if (auto p = std::find(ba.begin(), end, '+'); p != end)
			std::replace(p, p, '+', ' ');
	} else if (format == BarcodeFormat::Code93) {
		// Remove checksum digits
		if (ba.size() > 2)
			ba.erase(ba.end() - 2, ba.end());
	}

	SymbologyIdentifier si = SymbologyIdentifierZint2ZXing(opts, ba);
	Content content(std::move(ba), si, {}, eci);
	if (eci == ECI::Unknown)
		content.switchEncoding(CharacterSetFromString(std::to_string(zint->raw_segs[0].eci)));
	if (opts.encoding() != CharacterSet::Unknown)
		content.optionsCharset = opts.encoding();

	// Byte array `bits` has white as 0, black as non-zero (0xFF)
	auto bits = BitMatrix(zint->bitmap_width, zint->bitmap_height);
	std::transform(zint->bitmap, zint->bitmap + zint->bitmap_width * zint->bitmap_height, bits.row(0).begin(),
				   [](unsigned char v) { return (v != '0') * BitMatrix::SET_V; });

	auto res = Barcode(std::move(content), format, {} /*error*/, opts.readerInit(),
					   ECLevelZint2ZXing(zint), PositionZint2ZXing(opts, bits), VersionZint2ZXing(zint), DataMaskZint2ZXing(zint));
	res.symbol(std::move(bits));
	res.zint(std::move(opts.d->zint));

	// Reset zint for writing
	zint->show_hrt = 1;
	zint->output_options &= ~(OUT_BUFFER_INTERMEDIATE | BARCODE_RAW_TEXT);
	ZBarcode_Clear(zint);
	CHECK(ZBarcode_Encode(zint, (const uint8_t*)data, size));

	return res;
}

Barcode CreateBarcodeFromText(std::string_view contents, const CreatorOptions& opts)
{
	return CreateBarcode(contents.data(), contents.size(), -1 /*mode*/, opts);
}

#if __cplusplus > 201703L
Barcode CreateBarcodeFromText(std::u8string_view contents, const CreatorOptions& opts)
{
	return CreateBarcode(contents.data(), contents.size(), -1 /*mode*/, opts);
}
#endif

Barcode CreateBarcodeFromBytes(const void* data, int size, const CreatorOptions& opts)
{
	return CreateBarcode(data, size, DATA_MODE, opts);
}

// Writer ========================================================================

struct SetCommonWriterOptions
{
	zint_symbol* zint;
	int show_hrt;
	int output_options;
	float scale;
	std::string outfile;

	SetCommonWriterOptions(zint_symbol* zint, const WriterOptions& opts) : zint(zint)
	{
		// Save
		show_hrt = zint->show_hrt;
		output_options = zint->output_options;
		scale = zint->scale;
		outfile = zint->outfile;

		// Set
		zint->show_hrt = opts.withHRT();

		zint->output_options &= ~(BARCODE_QUIET_ZONES | BARCODE_NO_QUIET_ZONES);
		zint->output_options |= opts.withQuietZones() ? BARCODE_QUIET_ZONES : BARCODE_NO_QUIET_ZONES;

		if (opts.scale())
			zint->scale = opts.scale() / 2.f;
		else if (opts.sizeHint()) {
			int size = std::max(zint->width, zint->rows);
			zint->scale = std::max(1, int(float(opts.sizeHint()) / size)) / 2.f;
		}
	}

	// reset the defaults such that consecutive write calls don't influence each other
	~SetCommonWriterOptions()
	{
		zint->show_hrt = show_hrt;
		zint->output_options = output_options;
		zint->scale = scale;
		strcpy(zint->outfile, outfile.c_str());
	}
};

} // ZXing

#else // ZXING_USE_ZINT

#include "MultiFormatWriter.h"

namespace ZXing {

zint_symbol* CreatorOptions::zint() const { return nullptr; }

static Barcode CreateBarcode(BitMatrix&& bits, std::string_view contents, const CreatorOptions& opts)
{
	auto img = ToMatrix<uint8_t>(bits);

	auto res = Barcode(std::string(contents), 0, 0, 0, opts.format(), {});
	res.symbol(std::move(bits));
	return res;
}

Barcode CreateBarcodeFromText(std::string_view contents, const CreatorOptions& opts)
{
	auto writer = MultiFormatWriter(opts.format()).setMargin(0);
	if (!opts.ecLevel().empty())
		writer.setEccLevel(std::stoi(opts.ecLevel()));
	writer.setEncoding(CharacterSet::UTF8); // write UTF8 (ECI value 26) for maximum compatibility

	return CreateBarcode(writer.encode(std::string(contents), 0, IsLinearCode(opts.format()) ? 50 : 0), contents, opts);
}

#if __cplusplus > 201703L
Barcode CreateBarcodeFromText(std::u8string_view contents, const CreatorOptions& opts)
{
	return CreateBarcodeFromText({reinterpret_cast<const char*>(contents.data()), contents.size()}, opts);
}
#endif

Barcode CreateBarcodeFromBytes(const void* data, int size, const CreatorOptions& opts)
{
	std::wstring bytes;
	for (uint8_t c : std::basic_string_view<uint8_t>((uint8_t*)data, size))
		bytes.push_back(c);

	auto writer = MultiFormatWriter(opts.format()).setMargin(0);
	if (!opts.ecLevel().empty())
		writer.setEccLevel(std::stoi(opts.ecLevel()));
	writer.setEncoding(CharacterSet::BINARY);

	return CreateBarcode(writer.encode(bytes, 0, IsLinearCode(opts.format()) ? 50 : 0), std::string_view(reinterpret_cast<const char*>(data), size), opts);
}

} // namespace ZXing

#endif // ZXING_USE_ZINT

#else // ZXING_WRITERS

namespace ZXing {

zint_symbol* CreatorOptions::zint() const { return nullptr; }

Barcode CreateBarcodeFromText(std::string_view, const CreatorOptions&)
{
	throw std::runtime_error("This build of zxing-cpp does not support creating barcodes.");
}

#if __cplusplus > 201703L
Barcode CreateBarcodeFromText(std::u8string_view, const CreatorOptions&)
{
	throw std::runtime_error("This build of zxing-cpp does not support creating barcodes.");
}
#endif

Barcode CreateBarcodeFromBytes(const void*, int, const CreatorOptions&)
{
	throw std::runtime_error("This build of zxing-cpp does not support creating barcodes.");
}

} // namespace ZXing

#endif // ZXING_WRITERS

namespace ZXing {

std::string WriteBarcodeToSVG(const Barcode& barcode, [[maybe_unused]] const WriterOptions& opts)
{
	auto zint = barcode.zint();

	if (!zint)
		return ToSVG(barcode.symbol());

#if defined(ZXING_WRITERS) && defined(ZXING_USE_ZINT)
	auto resetOnExit = SetCommonWriterOptions(zint, opts);

	zint->output_options |= BARCODE_MEMORY_FILE;// | EMBED_VECTOR_FONT;
	strcpy(zint->outfile, "null.svg");

	CHECK(ZBarcode_Print(zint, opts.rotate()));

	return std::string(reinterpret_cast<const char*>(zint->memfile), zint->memfile_size);
#else
	return {}; // unreachable code
#endif
}

Image WriteBarcodeToImage(const Barcode& barcode, [[maybe_unused]] const WriterOptions& opts)
{
	auto zint = barcode.zint();

	if (!zint)
		return ToImage(barcode._symbol->copy(), IsLinearCode(barcode.format()), opts);

#if defined(ZXING_WRITERS) && defined(ZXING_USE_ZINT)
	auto resetOnExit = SetCommonWriterOptions(zint, opts);

	CHECK(ZBarcode_Buffer(zint, opts.rotate()));

#ifdef PRINT_DEBUG
	printf("write symbol with size: %dx%d\n", zint->bitmap_width, zint->bitmap_height);
#endif
	auto iv = Image(zint->bitmap_width, zint->bitmap_height);
	auto* src = zint->bitmap;
	auto* dst = const_cast<uint8_t*>(iv.data());
	for(int y = 0; y < iv.height(); ++y)
		for(int x = 0, w = iv.width(); x < w; ++x, src += 3)
			*dst++ = RGBToLum(src[0], src[1], src[2]);

	return iv;
#else
	return {}; // unreachable code
#endif
}

std::string WriteBarcodeToUtf8(const Barcode& barcode, [[maybe_unused]] const WriterOptions& options)
{
	auto iv = barcode.symbol();
	if (!iv.data())
		return {};

	constexpr auto map = std::array{" ", "▀", "▄", "█"};
	std::ostringstream res;
	bool inverted = false; // TODO: take from WriterOptions

	for (int y = 0; y < iv.height(); y += 2) {
		for (int x = 0; x < iv.width(); ++x) {
			int tp = bool(*iv.data(x, y)) ^ inverted;
			int bt = (iv.height() == 1 && tp) || (y + 1 < iv.height() && (bool(*iv.data(x, y + 1)) ^ inverted));
			res << map[tp | (bt << 1)];
		}
		res << '\n';
	}

	return res.str();
}

} // namespace ZXing

#endif // ZXING_EXPERIMENTAL_API
