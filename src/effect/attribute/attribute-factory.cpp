#include "effect/attribute/attribute-factory.h"
#include "effect/attribute/abstract-attribute.h"

std::unique_ptr<AbstractAttribute> AttributeFactory::create_attribute(AttributeType)
{
    return nullptr;
}
