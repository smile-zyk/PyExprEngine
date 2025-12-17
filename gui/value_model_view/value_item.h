#pragma once

#include <cstddef>
#include <memory>
#include <vector>
#include <QString>
#include <tsl/ordered_map.h>
#include <core/value.h>

namespace xequation
{
namespace gui
{
class ValueItem
{
public:
    using UniquePtr = std::unique_ptr<ValueItem>;
    ~ValueItem() = default;
    static UniquePtr Create(const QString &name, const Value &value, ValueItem* parent = nullptr);
    static UniquePtr Create(const QString &name, const QString& display_value, const QString& type, ValueItem* parent = nullptr);
    void LoadChildren(int begin, int end);
    void UnLoadChildren();
    void AddChild(ValueItem::UniquePtr child);
    void RemoveChild(ValueItem* child);
    ValueItem* GetChildAt(int index) const;
    int GetIndexOfChild(ValueItem* child) const;
    bool HasChildren() const { return expected_child_count_ > 0; }
    bool IsLoaded() const { return expected_child_count() == loaded_child_count(); }

    void set_type(const QString &type) { type_ = type; }
    void set_display_value(const QString &display_value) { display_value_ = display_value; }
    void set_expected_count(size_t count) { expected_child_count_ = count; }
    
    const QString& name() const { return name_; }
    const Value& value() const { return value_; }
    const QString& type() const { return type_; }
    const QString& display_value() const { return display_value_; }
    ValueItem* parent() const { return parent_; }
    size_t expected_child_count() const { return expected_child_count_; }
    size_t loaded_child_count() const { return children_.size(); }

protected:
    ValueItem(const QString &name, const Value &value, ValueItem* parent = nullptr);
    ValueItem(const QString &name, const QString& display_value, const QString& type, ValueItem* parent = nullptr);
private:
    QString name_;
    Value value_;
    QString type_;
    QString display_value_;
    ValueItem* parent_ = nullptr;
    std::vector<std::unique_ptr<ValueItem>> children_;
    size_t expected_child_count_ = 0;
};
}
}