#pragma once
#include <QMap>
#include <QObject>
#include <QString>

namespace xequation
{
namespace gui
{

class VariableModelDataManager;

class VariableModelData
{
  public:
    VariableModelData(const QString &name, const QString &value, const QString &type);
    ~VariableModelData();

    void AddChild(VariableModelData *child);
    void AddChildren(const QList<VariableModelData *> &children);
    void RemoveChild(const QString &name);
    void RemoveChild(VariableModelData *child);
    VariableModelData *GetChild(const QString &name) const;
    VariableModelData *GetChildAt(int index) const;
    void ClearChildren();
    int IndexOfChild(VariableModelData *child) const;

    void set_value(const QString &value)
    {
        value_ = value;
    }

    void set_type(const QString &type)
    {
        type_ = type;
    }

    const QString &name() const
    {
        return name_;
    }

    const QString &value() const
    {
        return value_;
    }

    const QString &type() const
    {
        return type_;
    }

    VariableModelData *parent() const
    {
        return parent_;
    }

    int ChildCount() const
    {
        return child_list_.size();
    }
    
    QList<VariableModelData *> children() const
    {
        return child_list_;
    }

    VariableModelDataManager* manager()
    {
        return manager_;
    }

  private:
    friend class VariableModelDataManager;
    QString name_;
    QString value_;
    QString type_;
    QList<VariableModelData*> child_list_;
    QMap<QString, VariableModelData *> child_map_;
    VariableModelData *parent_;
    VariableModelDataManager* manager_;
};

class VariableModelDataManager : public QObject
{
    Q_OBJECT
  public:
    VariableModelDataManager(QObject* parent = nullptr);
    ~VariableModelDataManager();

    VariableModelData *CreateData(const QString &name, const QString &value, const QString &type);
    VariableModelData *GetData(const QString &name) const;
    bool RemoveData(const QString &name);
    bool RemoveData(VariableModelData* data);
    void Clear();

    QList<QString> GetAllNames() const
    {
        return data_map_.keys();
    }
    int Count() const
    {
        return data_map_.size();
    }

  private:
    QMap<QString, VariableModelData*> data_map_;
};

} // namespace gui
} // namespace xequation
