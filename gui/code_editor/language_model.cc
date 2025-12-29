#include "language_model.h"
#include <QFile>
#include <QLanguage>
#include <qglobal.h>
#include <qnamespace.h>

namespace xequation
{
namespace gui
{

QMap<QString, QString> LanguageModel::language_define_file_map_ = {
    {"Python", ":/languages/python.xml"},
};

LanguageModel::LanguageModel(const QString &language_name, QObject *parent)
    : QAbstractListModel(parent), language_name_(language_name)
{
    auto it = language_define_file_map_.find(language_name);
    if (it == language_define_file_map_.end())
    {
        return;
    }

    QString define_file = it.value();
    QFile fl(define_file);

    if (!fl.open(QIODevice::ReadOnly))
    {
        return;
    }

    QLanguage language(&fl);

    if (!language.isLoaded())
    {
        return;
    }

    auto keys = language.keys();
    for (auto &&key : keys)
    {
        auto names = language.names(key);
        for (auto &&name : names)
        {
            WordItem item;
            item.word = name;
            item.category = key;
            item.complete_content = name;
            word_items_map_[key].insert(item);
            word_item_set_.insert(name);
            language_item_set_.insert(name);
            total_word_count_++;
        }
    }
}

LanguageModel::~LanguageModel() {}

int LanguageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return total_word_count_
}

QVariant LanguageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= total_word_count_)
    {
        return QVariant();
    }

    const WordItem &item = word_items_map_.at(index.row());
    // display "{word}\t{category}"
    if (role == Qt::DisplayRole)
    {
        return item.word + "    " + item.category.toLower();
    }
    else if (role == Qt::EditRole)
    {
        return item.complete_content;
    }
    else if (role == kWordRole)
    {
        return item.word;
    }
    else if (role == kCategoryRole)
    {
        return item.category;
    }

    return QVariant();
}

void LanguageModel::AddWordItem(const QString &word, const QString &category, const QString &complete_content)
{
    if (word_item_set_.contains(word))
    {
        return;
    }

    beginInsertRows(QModelIndex(), word_items_.size(), word_items_.size());
    WordItem item;
    item.word = word;
    item.category = category;
    item.complete_content = complete_content;
    word_items_.append(item);
    word_item_set_.insert(word);
    endInsertRows();

    category_to_word_items_map_[category].insert(&word_items_.back());
}

void LanguageModel::RemoveWordItem(const QString &word)
{
    if (!word_item_set_.contains(word) || language_item_set_.contains(word))
    {
        return;
    }
    word_item_set_.remove(word);
    for (int i = 0; i < word_items_.size(); ++i)
    {
        if (word_items_[i].word == word)
        {
            beginRemoveRows(QModelIndex(), i, i);
            word_items_.removeAt(i);
            language_item_set_.remove(word);
            endRemoveRows();
            break;
        }
    }
}
} // namespace gui
} // namespace xequation