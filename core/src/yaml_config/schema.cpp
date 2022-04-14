#include <userver/yaml_config/schema.hpp>

#include <unordered_set>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/consteval_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace yaml_config {

namespace {

constexpr auto kNamesToTypes =
    utils::MakeConsinitMap<std::string_view, FieldType>({
        {"integer", FieldType::kInt},
        {"string", FieldType::kString},
        {"boolean", FieldType::kBool},
        {"double", FieldType::kDouble},
        {"object", FieldType::kObject},
        {"array", FieldType::kArray},
    });

constexpr auto kSchemaFields = utils::MakeConsinitSet<std::string_view>({
    {"type"},
    {"description"},
    {"defaultDescription"},
    {"additionalProperties"},
    {"properties"},
    {"items"},
});

void CheckFieldsNames(const formats::yaml::Value& yaml_schema) {
  for (const auto& [name, value] : Items(yaml_schema)) {
    if (!kSchemaFields.Contains(name)) {
      throw std::runtime_error(fmt::format(
          "Schema field name must be one of ['type', 'description', "
          "'defaultDescription', 'additionalProperties', 'properties', "
          "'items'], but '{}' was given. Schema path: '{}'",
          name, yaml_schema.GetPath()));
    }
  }
}

void CheckSchemaStructure(const Schema& schema) {
  if (schema.items.has_value() && schema.type != FieldType::kArray) {
    throw std::runtime_error(
        fmt::format("Schema field '{}' of type '{}' can not have field "
                    "'items', because its type is not 'array'",
                    schema.path, ToString(schema.type)));
  }
  if (schema.type != FieldType::kObject) {
    if (schema.properties.has_value()) {
      throw std::runtime_error(
          fmt::format("Schema field '{}' of type '{}' can not have field "
                      "'properties', because its type is not 'object'",
                      schema.path, ToString(schema.type)));
    }
    if (schema.additional_properties.has_value()) {
      throw std::runtime_error(
          fmt::format("Schema field '{}' of type '{}' can not have field "
                      "'additionalProperties, because its type is not 'object'",
                      schema.path, ToString(schema.type)));
    }
  }

  if (schema.type == FieldType::kObject) {
    if (!schema.properties.has_value()) {
      throw std::runtime_error(
          fmt::format("Schema field '{}' of type 'object' "
                      "must have field 'properties'",
                      schema.path));
    }
    if (!schema.additional_properties.has_value()) {
      throw std::runtime_error(
          fmt::format("Schema field '{}' of type 'object' must have field "
                      "'additionalProperties'",
                      schema.path));
    }
  } else if (schema.type == FieldType::kArray) {
    if (!schema.items.has_value()) {
      throw std::runtime_error(fmt::format(
          "Schema field '{}' of type 'array' must have field 'items'",
          schema.path));
    }
  }
}

}  // namespace

std::string ToString(FieldType type) {
  switch (type) {
    case FieldType::kInt:
      return "integer";
    case FieldType::kString:
      return "string";
    case FieldType::kBool:
      return "boolean";
    case FieldType::kDouble:
      return "double";
    case FieldType::kObject:
      return "object";
    case FieldType::kArray:
      return "array";
    default:
      UINVARIANT(false, "Incorrect field type");
  }
}

FieldType Parse(const formats::yaml::Value& type,
                formats::parse::To<FieldType>) {
  const std::string as_string = type.As<std::string>();
  const auto ptr = kNamesToTypes.FindOrNullptr(as_string);

  if (ptr) {
    return *ptr;
  }

  throw std::runtime_error(
      fmt::format("Schema field 'type' must be one of ['integer', "
                  "'string' 'boolean', 'object', 'array']), but '{}' was given",
                  as_string));
}

SchemaPtr Parse(const formats::yaml::Value& schema,
                formats::parse::To<SchemaPtr>) {
  return SchemaPtr(schema.As<Schema>());
}

SchemaPtr::SchemaPtr(Schema&& schema)
    : schema_(std::make_unique<Schema>(std::move(schema))) {}

std::variant<bool, SchemaPtr> Parse(
    const formats::yaml::Value& value,
    formats::parse::To<std::variant<bool, SchemaPtr>>) {
  if (value.IsBool()) {
    return value.As<bool>();
  } else {
    return value.As<SchemaPtr>();
  }
}

Schema Parse(const formats::yaml::Value& schema, formats::parse::To<Schema>) {
  Schema result;
  result.path = schema.GetPath();
  result.type = schema["type"].As<FieldType>();
  result.description = schema["description"].As<std::string>();

  result.additional_properties =
      schema["additionalProperties"]
          .As<std::optional<std::variant<bool, SchemaPtr>>>();
  result.default_description =
      schema["defaultDescription"].As<std::optional<std::string>>();
  result.properties =
      schema["properties"]
          .As<std::optional<std::unordered_map<std::string, SchemaPtr>>>();
  result.items = schema["items"].As<std::optional<SchemaPtr>>();

  CheckFieldsNames(schema);

  CheckSchemaStructure(result);

  return result;
}

void Schema::UpdateDescription(std::string new_description) {
  description = std::move(new_description);
}

Schema impl::SchemaFromString(const std::string& yaml_string) {
  return formats::yaml::FromString(yaml_string).As<Schema>();
}

}  //  namespace yaml_config

USERVER_NAMESPACE_END
