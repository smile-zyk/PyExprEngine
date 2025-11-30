#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QVector>

namespace xequation
{
namespace gui
{
class VariableModelData;
class VariableModelDataManager;
class VariableModel : public QAbstractItemModel
{
    Q_OBJECT
  public:
    explicit VariableModel(QObject *parent = nullptr);
    ~VariableModel() override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void SetRootData(const QVector<VariableModelData *> &root_data);
    void AddRootData(VariableModelData *data);
    void RemoveRootData(VariableModelData *data);
    void ClearRootData();
    VariableModelData *GetRootData() const;

  private:
    QModelIndex CreateIndexFromData(VariableModelData *data, int row, int column) const;
    VariableModelData *GetDataFromIndex(const QModelIndex &index) const;
    int RowOfChildInParent(VariableModelData *parent, VariableModelData *child) const;

  private:
    QVector<VariableModelData *> root_data_;
};

} // namespace gui
} // namespace xequation