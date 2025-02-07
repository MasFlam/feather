// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2023 The Monero Project

#include "SubaddressModel.h"
#include "Subaddress.h"

#include <QPoint>
#include <QColor>
#include <QBrush>

#include "utils/ColorScheme.h"
#include "utils/Utils.h"

SubaddressModel::SubaddressModel(QObject *parent, Subaddress *subaddress)
    : QAbstractTableModel(parent)
    , m_subaddress(subaddress)
    , m_showFullAddresses(false)
{
    connect(m_subaddress, &Subaddress::refreshStarted, this, &SubaddressModel::startReset);
    connect(m_subaddress, &Subaddress::refreshFinished, this, &SubaddressModel::endReset);
}

void SubaddressModel::startReset(){
    beginResetModel();
}

void SubaddressModel::endReset(){
    endResetModel();
}

int SubaddressModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    } else {
        return m_subaddress->count();
    }
}

int SubaddressModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return ModelColumn::COUNT;
}

QVariant SubaddressModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || static_cast<quint64>(index.row()) >= m_subaddress->count())
        return QVariant();

    QVariant result;

    bool found = m_subaddress->getRow(index.row(), [this, &index, &role, &result](const Monero::SubaddressRow &subaddress) {
        if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::UserRole){
            result = parseSubaddressRow(subaddress, index, role);
        }
        else if (role == Qt::BackgroundRole) {
            switch(index.column()) {
                case Address:
                {
                    if (subaddress.isUsed()) {
                        result = QBrush(ColorScheme::RED.asColor(true));
                    }
                }
            }
        }
        else if (role == Qt::FontRole) {
            switch(index.column()) {
                case Address:
                {
                   result = Utils::getMonospaceFont();
                }
            }
        }
        else if (role == Qt::ToolTipRole) {
            switch(index.column()) {
                case Address:
                {
                    if (subaddress.isUsed()) {
                        result = "This address is used.";
                    }
                }
            }
        }
    });

    if (!found)
    {
        qCritical("%s: internal error: invalid index %d", __FUNCTION__, index.row());
    }

    return result;
}

QVariant SubaddressModel::parseSubaddressRow(const Monero::SubaddressRow &subaddress, const QModelIndex &index, int role) const
{
    switch (index.column()) {
        case Index:
        {
            return "#" + QString::number(subaddress.getRowId()) + " ";
        }
        case Address:
        {
            QString address = QString::fromStdString(subaddress.getAddress());
            if (!m_showFullAddresses && role != Qt::UserRole) {
                address = Utils::displayAddress(address);
            }
            return address;
        }
        case Label:
        {
            if (m_currentSubaddressAccount == 0 && index.row() == 0) {
                return "Primary address";
            }
            else if (index.row() == 0) {
                return "Change";
            }
            return QString::fromStdString(subaddress.getLabel());
        }
        case isUsed:
            return subaddress.isUsed();
        default:
            qCritical() << "Invalid column" << index.column();
            return QVariant();
    }
}


QVariant SubaddressModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }
    if (orientation == Qt::Horizontal)
    {
        switch(section) {
            case Index:
                return QString("#");
            case Address:
                return QString("Address");
            case Label:
                return QString("Label");
            case isUsed:
                return QString("Used");
            default:
                return QVariant();
        }
    }
    return QVariant();
}

bool SubaddressModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {
        const int row = index.row();

        switch (index.column()) {
            case Label:
                m_subaddress->setLabel(m_currentSubaddressAccount, row, value.toString());
                break;
            default:
                return false;
        }
        emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});

        return true;
    }
    return false;
}

void SubaddressModel::setShowFullAddresses(bool show)
{
    m_showFullAddresses = show;
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

Qt::ItemFlags SubaddressModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    if (index.column() == Label && index.row() != 0)
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;

    return QAbstractTableModel::flags(index);
}

bool SubaddressModel::isShowFullAddresses() const {
    return m_showFullAddresses;
}

int SubaddressModel::unusedLookahead() const {
    return m_subaddress->unusedLookahead();
}

void SubaddressModel::setCurrentSubaddressAccount(quint32 accountIndex) {
    m_currentSubaddressAccount = accountIndex;
}

Monero::SubaddressRow* SubaddressModel::entryFromIndex(const QModelIndex &index) const {
    Q_ASSERT(index.isValid() && index.row() < m_subaddress->count());
    return m_subaddress->row(index.row());
}