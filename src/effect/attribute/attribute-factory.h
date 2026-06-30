#pragma once

#include <memory>

class AbstractAttribute;
enum class AttributeType;

class AttributeFactory {
public:
    static std::unique_ptr<AbstractAttribute> create_attribute(AttributeType typ);
};
