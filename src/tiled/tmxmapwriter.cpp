/*
 * tmxmapwriter.cpp
 * Copyright 2008-2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "tmxmapwriter.h"

#include "mapwriter.h"
#include "preferences.h"

#include <QBuffer>

using namespace Tiled;
using namespace Tiled::Internal;

bool TmxMapWriter::write(const Map *map, const QString &fileName)
{
    Preferences *prefs = Preferences::instance();

    MapWriter writer;
    writer.setLayerDataFormat(prefs->layerDataFormat());
    writer.setDtdEnabled(prefs->dtdEnabled());

    bool result = writer.writeMap(map, fileName);
    if (!result)
        mError = writer.errorString();
    else
        mError.clear();

    return result;
}

bool TmxMapWriter::writeTileset(const Tileset *tileset,
                                const QString &fileName)
{
    QFile file(fileName);
    QXmlStreamWriter *writer = createWriter(&file);
    if (!writer)
        return false;

    writer->writeStartDocument();

    if (mDtdEnabled) {
        writer->writeDTD(QLatin1String("<!DOCTYPE tileset SYSTEM \""
                                       "http://mapeditor.org/dtd/1.0/"
                                       "map.dtd\">"));
    }

    writeTileset(*writer, tileset, 0);
    writer->writeEndDocument();
    delete writer;

    if (file.error() != QFile::NoError) {
        mError = file.errorString();
        return false;
    }

    return true;
}

QString TmxMapWriter::toString(const Map *map)
{
    mUseAbsolutePaths = true;

    QString output;
    QXmlStreamWriter writer(&output);

    writer.writeStartDocument();
    writeMap(writer, map);
    writer.writeEndDocument();

    return output;
}

void TmxMapWriter::writeMap(QXmlStreamWriter &w, const Map *map)
{
    w.writeStartElement(QLatin1String("map"));

    QString orientation;
    switch (map->orientation()) {
    case Map::Orthogonal:
        orientation = QLatin1String("orthogonal");
        break;
    case Map::Isometric:
        orientation = QLatin1String("isometric");
        break;
    case Map::Hexagonal:
        orientation = QLatin1String("hexagonal");
        break;
    case Map::Unknown:
        break;
    }

    w.writeAttribute(QLatin1String("version"), QLatin1String("1.0"));
    if (!orientation.isEmpty())
        w.writeAttribute(QLatin1String("orientation"), orientation);
    w.writeAttribute(QLatin1String("top"), QString::number(map->size().top()));
    w.writeAttribute(QLatin1String("left"), QString::number(map->size().left()));
    w.writeAttribute(QLatin1String("width"), QString::number(map->size().width()));
    w.writeAttribute(QLatin1String("height"), QString::number(map->size().height()));
    w.writeAttribute(QLatin1String("tilewidth"),
                     QString::number(map->tileWidth()));
    w.writeAttribute(QLatin1String("tileheight"),
                     QString::number(map->tileHeight()));

    writeProperties(w, map->properties());

    Preferences *prefs = Preferences::instance();


    MapWriter writer;
    writer.setDtdEnabled(prefs->dtdEnabled());

    bool result = writer.writeTileset(tileset, fileName);
    if (!result)
        mError = writer.errorString();
    else
        mError.clear();

    return result;
}

QByteArray TmxMapWriter::toByteArray(const Map *map)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);

    MapWriter writer;
    writer.setLayerDataFormat(MapWriter::Base64Zlib);
    writer.writeMap(map, &buffer);

    return bytes;
}
