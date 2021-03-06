/*
 * map.h
 * Copyright 2008-2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2008, Roderic Morris <roderic@ccs.neu.edu>
 * Copyright 2010, Andrew G. Crowell <overkill9999@gmail.com>
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

#ifndef MAP_H
#define MAP_H

#include "object.h"

#include <QList>
#include <QSize>
#include <QRect>

namespace Tiled {

class Layer;
class Tile;
class Tileset;
class ObjectGroup;

/**
 * A tile map. Consists of a stack of layers, each can be either a TileLayer
 * or an ObjectGroup.
 *
 * It also keeps track of the list of referenced tilesets.
 */
class TILEDSHARED_EXPORT Map : public Object
{
public:
    /**
     * The orientation of the map determines how it should be rendered. An
     * Orthogonal map is using rectangular tiles that are aligned on a
     * straight grid. An Isometric map uses diamond shaped tiles that are
     * aligned on an isometric projected grid. A Hexagonal map uses hexagon
     * shaped tiles that fit into each other by shifting every other row.
     *
     * Only Orthogonal and Isometric maps are supported by this version of
     * Tiled.
     */
    enum Orientation {
        Unknown,
        Orthogonal,
        Isometric,
        Hexagonal
    };

    /**
     * Constructor, taking map orientation, size and tile size as parameters.
     */
    Map(Orientation orientation,
        QRect size,
        int tileWidth, int tileHeight);

    /**
     * Destructor.
     */
    ~Map();

    /**
     * Returns the orientation of the map.
     */
    Orientation orientation() const { return mOrientation; }

    /**
     * Sets the orientation of the map.
     */
    void setOrientation(Orientation orientation)
    { mOrientation = orientation; }

    /**
     * Returns the size of this map.
     */
    QRect size() const { return mSize; }
    
    /**
     * Sets the size of this map.
     */
    void setSize(const QRect &size) { mSize = size; } // this is perhaps of dubious reliability

    /**
     * Returns the tile width of this map.
     */
    int tileWidth() const { return mTileWidth; }

    /**
     * Returns the tile height used by this map.
     */
    int tileHeight() const { return mTileHeight; }

    /**
     * Returns the maximum tile size used by tile layers of this map.
     * @see TileLayer::extraTileSize()
     */
    QSize maxTileSize() const { return mMaxTileSize; }

    /**
     * Adjusts the maximum tile size to be at least as much as the given
     * size. Called from tile layers when their maximum tile size increases.
     */
    void adjustMaxTileSize(const QSize &size);

    /**
     * Convenience method for getting the extra tile size, which is the number
     * of pixels that tiles may extend beyond the size of the tile grid.
     *
     * @see maxTileSize()
     */
    QSize extraTileSize() const
    { return QSize(mMaxTileSize.width() - mTileWidth,
                   mMaxTileSize.height() - mTileHeight); }

    /**
     * Returns the number of layers of this map.
     */
    int layerCount() const
    { return mLayers.size(); }

    /**
     * Convenience function that returns the number of layers of this map that
     * are tile layers.
     */
    int tileLayerCount() const;

    /**
     * Convenience function that returns the number of layers of this map that
     * are object groups.
     */
    int objectGroupCount() const;

    /**
     * Returns the layer at the specified index.
     */
    Layer *layerAt(int index) const
    { return mLayers.at(index); }

    /**
     * Returns the list of layers of this map. This is useful when you want to
     * use foreach.
     */
    const QList<Layer*> &layers() const { return mLayers; }

    /**
     * Adds a layer to this map.
     */
    void addLayer(Layer *layer);

    /**
     * Returns the index of the layer given by \a layerName, or -1 if no
     * layer with that name is found.
     */
    int indexOfLayer(const QString &layerName) const;

    /**
     * Adds a layer to this map, inserting it at the given index.
     */
    void insertLayer(int index, Layer *layer);

    /**
     * Removes the layer at the given index from this map and returns it.
     * The caller becomes responsible for the lifetime of this layer.
     */
    Layer *takeLayerAt(int index);

    /**
     * Adds a tileset to this map. The map does not take ownership over its
     * tilesets, this is merely for keeping track of which tilesets are used by
     * the map, and their saving order.
     *
     * @param tileset the tileset to add
     */
    void addTileset(Tileset *tileset);

    /**
     * Inserts \a tileset at \a index in the list of tilesets used by this map.
     */
    void insertTileset(int index, Tileset *tileset);

    /**
     * Returns the index of the given \a tileset, or -1 if it is not used in
     * this map.
     */
    int indexOfTileset(Tileset *tileset) const;

    /**
     * Removes the tileset at \a index from this map.
     *
     * \warning Does not make sure that this map no longer refers to tiles from
     *          the removed tileset!
     *
     * \sa addTileset
     */
    void removeTilesetAt(int index);

    /**
     * Replaces all tiles from \a oldTileset with tiles from \a newTileset.
     * Also replaces the old tileset with the new tileset in the list of
     * tilesets.
     */
    void replaceTileset(Tileset *oldTileset, Tileset *newTileset);

    /**
     * Returns the tilesets that the tiles on this map are using.
     */
    const QList<Tileset*> &tilesets() const { return mTilesets; }

    /**
     * Returns whether the given \a tileset is used by any tile layer of this
     * map.
     */
    bool isTilesetUsed(Tileset *tileset) const;

    Map *clone() const;

private:
    void adoptLayer(Layer *layer);

    Orientation mOrientation;
    QRect mSize;
    int mTileWidth;
    int mTileHeight;
    QSize mMaxTileSize;
    QList<Layer*> mLayers;
    QList<Tileset*> mTilesets;
};

} // namespace Tiled

#endif // MAP_H
