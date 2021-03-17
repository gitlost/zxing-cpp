/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "ResultMetadata.h"

#include "ByteArray.h"

#include <utility>

namespace ZXing {

struct ResultMetadata::Value
{
	virtual ~Value() = default;
	virtual int toInteger(int fallback) const {
		return fallback;
	}
	virtual bool toBool() const {
		return false;
	}
	virtual std::string toNString() const {
		return std::string();
	}
	virtual std::wstring toString() const {
		return std::wstring();
	}
	virtual std::list<ByteArray> toByteArrayList() const {
		return std::list<ByteArray>();
	}
	virtual std::shared_ptr<CustomData> toCustomData() const {
		return nullptr;
	}
	virtual std::list<std::string> toStringList() const {
		return std::list<std::string>();
	}
};

struct ResultMetadata::IntegerValue : public Value
{
	int value;
	explicit IntegerValue(int v) : value(v) {}
	int toInteger(int) const override {
		return value;
	}
	std::wstring toString() const override {
		return std::to_wstring(value);
	}
};

struct ResultMetadata::BoolValue : public Value
{
	bool value;
	explicit BoolValue(bool v) : value(v) {}
	bool toBool() const override {
		return value;
	}
	std::wstring toString() const override {
		return std::to_wstring(value);
	}
};

struct ResultMetadata::NStringValue : public Value
{
	std::string value;
	explicit NStringValue(std::string v) : value(std::move(v)) {}
	std::string toNString() const override {
		return value;
	}
	std::wstring toString() const override {
		return std::wstring(value.begin(), value.end());
	}
};

struct ResultMetadata::StringValue : public Value
{
	std::wstring value;
	explicit StringValue(std::wstring v) : value(std::move(v)) {}
	std::wstring toString() const override {
		return value;
	}
};

struct ResultMetadata::ByteArrayListValue : public Value
{
	std::list<ByteArray> value;
	explicit ByteArrayListValue(std::list<ByteArray> v) : value(std::move(v)) {}
	std::list<ByteArray> toByteArrayList() const override {
		return value;
	}
};

struct ResultMetadata::CustomDataValue : public Value
{
	std::shared_ptr<CustomData> value;
	explicit CustomDataValue(std::shared_ptr<CustomData> v) : value(std::move(v)) {}
	std::shared_ptr<CustomData> toCustomData() const override {
		return value;
	}
};

struct ResultMetadata::StringListValue : public Value
{
	std::list<std::string> value;
	explicit StringListValue(std::list<std::string> v) : value(std::move(v)) {}
	std::list<std::string> toStringList() const override {
		return value;
	}
};

int
ResultMetadata::getInt(Key key, int fallbackValue) const
{
	auto it = _contents.find(key);
	return it != _contents.end() ? it->second->toInteger(fallbackValue) : fallbackValue;
}

bool
ResultMetadata::getBool(Key key) const
{
	auto it = _contents.find(key);
	return it != _contents.end() ? it->second->toBool() : false;
}

std::string
ResultMetadata::getNString(Key key) const
{
	auto it = _contents.find(key);
	return it != _contents.end() ? it->second->toNString() : std::string();
}

std::wstring
ResultMetadata::getString(Key key) const
{
	auto it = _contents.find(key);
	return it != _contents.end() ? it->second->toString() : std::wstring();
}

std::list<ByteArray>
ResultMetadata::getByteArrayList(Key key) const
{
	auto it = _contents.find(key);
	return it != _contents.end() ? it->second->toByteArrayList() : std::list<ByteArray>();
}

std::shared_ptr<CustomData>
ResultMetadata::getCustomData(Key key) const
{
	auto it = _contents.find(key);
	return it != _contents.end() ? it->second->toCustomData() : nullptr;
}

std::list<std::string>
ResultMetadata::getStringList(Key key) const
{
	auto it = _contents.find(key);
	return it != _contents.end() ? it->second->toStringList() : std::list<std::string>();
}

void
ResultMetadata::put(Key key, int value)
{
	_contents[key] = std::make_shared<IntegerValue>(value);
}

void
ResultMetadata::put(Key key, bool value)
{
	_contents[key] = std::make_shared<BoolValue>(value);
}

void
ResultMetadata::put(Key key, const std::string& value)
{
	_contents[key] = std::make_shared<NStringValue>(value);
}

void
ResultMetadata::put(Key key, const std::wstring& value)
{
	_contents[key] = std::make_shared<StringValue>(value);
}

void
ResultMetadata::put(Key key, const std::list<ByteArray>& value)
{
	_contents[key] = std::make_shared<ByteArrayListValue>(value);
}

void
ResultMetadata::put(Key key, const std::shared_ptr<CustomData>& value)
{
	_contents[key] = std::make_shared<CustomDataValue>(value);
}

void
ResultMetadata::put(Key key, const std::list<std::string>& value)
{
	_contents[key] = std::make_shared<StringListValue>(value);
}

void
ResultMetadata::putAll(const ResultMetadata& other)
{
	_contents.insert(other._contents.begin(), other._contents.end());
}

} // ZXing
