#include "effect/attribute/attribute-factory.h"
#include "effect/attribute-types.h"
#include "effect/attribute/abstract-attribute.h"
#include "effect/attribute/fire-attribute.h"

std::unique_ptr<AbstractAttribute> AttributeFactory::create_attribute(AttributeType typ)
{
    switch (typ) {
    case AttributeType::FIRE:
        return std::make_unique<FireAttribute>();
    default:
        return nullptr;
    }
}
