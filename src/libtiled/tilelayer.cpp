/*
 * tilelayer.cpp
 * Copyright 2008-2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2009, Jeff Bland <jksb@member.fsf.org>
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

#include "tilelayer.h"

#include "map.h"
#include "tile.h"
#include "tileset.h"

using namespace Tiled;

TileLayer::TileLayer(const QString &name, int x, int y, QRect size):
    Layer(name, x, y, size),
    mMaxTileSize(0, 0)
{
}

QRegion TileLayer::region() const
{
    QRegion region(size());

    for (int y = mSize.top(); y <= mSize.bottom(); ++y)
        for (int x = mSize.left(); x <= mSize.right(); ++x)
            if (!tileAt(x, y))
                region -= QRegion(x + mX, y + mY, 1, 1);

    return region;
}

Tile *TileLayer::tileAt(int x, int y) const
{
    std::map<int, std::map<int, Tile *> >::const_iterator it = mTiles.find(y);
    if (it == mTiles.end()) return NULL;
  
    std::map<int, Tile *>::const_iterator it2 = it->second.find(x);
    if (it2 == it->second.end()) return NULL;
  
    return it2->second;
}

void TileLayer::setTile(int x, int y, Tile *tile)
{
    if (tile) {
        if (tile->width() > mMaxTileSize.width()) {
            mMaxTileSize.setWidth(tile->width());
            if (mMap)
                mMap->adjustMaxTileSize(mMaxTileSize);
        }
        if (tile->height() > mMaxTileSize.height()) {
            mMaxTileSize.setHeight(tile->height());
            if (mMap)
                mMap->adjustMaxTileSize(mMaxTileSize);
        }
    }

    // save a little RAM
    if (tile) {
      mSize = mSize.united(QRect(x, y, 1, 1));
      
      mTiles[y][x] = tile;
    } else {
      mTiles[y].erase(x);
    }
}

TileLayer *TileLayer::copy(const QRegion &region) const
{
    const QRegion area = region.intersected(bounds());
    const QRect bounds = region.boundingRect();
    const QRect areaBounds = area.boundingRect();
    const int offsetX = qMax(0, areaBounds.x() - bounds.x());
    const int offsetY = qMax(0, areaBounds.y() - bounds.y());

    TileLayer *copied = new TileLayer(QString(),
                                      0, 0,
                                      bounds.intersected(mSize));

    foreach (const QRect &rect, area.rects())
        for (int x = rect.left(); x <= rect.right(); ++x)
            for (int y = rect.top(); y <= rect.bottom(); ++y)
                copied->setTile(x - areaBounds.x() + offsetX,
                                y - areaBounds.y() + offsetY,
                                tileAt(x, y));

    return copied;
}

void TileLayer::merge(const QPoint &pos, const TileLayer *layer)
{
    // Determine the overlapping area
    QRect area = QRect(pos, QSize(layer->width(), layer->height()));

    for (int y = area.top(); y <= area.bottom(); ++y)
        for (int x = area.left(); x <= area.right(); ++x)
            if (Tile *tile = layer->tileAt(x - area.left(), y - area.top()))
                setTile(x, y, tile);
}

QSet<Tileset*> TileLayer::usedTilesets() const
{
    QSet<Tileset*> tilesets;

    for (int i = 0, i_end = mTiles.size(); i < i_end; ++i)
        if (const Tile *tile = mTiles.at(i))
            tilesets.insert(tile->tileset());

    return tilesets;
}

bool TileLayer::referencesTileset(Tileset *tileset) const
{
    for (int y = mSize.top(); y <= mSize.bottom(); ++y) {
        for (int x = mSize.left(); x <= mSize.right(); ++x) {
            const Tile *tile = tileAt(x, y);
            if (tile && tile->tileset() == tileset)
                return true;
        }
    }
    return false;
}

QRegion TileLayer::tilesetReferences(Tileset *tileset) const
{
    QRegion region;

    for (int y = mSize.top(); y <= mSize.bottom(); ++y)
        for (int x = mSize.left(); x <= mSize.right(); ++x)
            if (const Tile *tile = tileAt(x, y))
                if (tile->tileset() == tileset)
                    region += QRegion(x + mX, y + mY, 1, 1);

    return region;
}

void TileLayer::removeReferencesToTileset(Tileset *tileset)
{
    for (int y = mSize.top(); y <= mSize.bottom(); ++y) {
        for (int x = mSize.left(); x <= mSize.right(); ++x) {
            const Tile *tile = tileAt(x, y);
            if (tile && tile->tileset() == tileset)
                setTile(x, y, NULL);
        }
    }
}

void TileLayer::replaceReferencesToTileset(Tileset *oldTileset,
                                           Tileset *newTileset)
{
    for (int y = mSize.top(); y <= mSize.bottom(); ++y) {
        for (int x = mSize.left(); x <= mSize.right(); ++x) {
            const Tile *tile = tileAt(x, y);
            if (tile && tile->tileset() == oldTileset)
                setTile(x, y, newTileset->tileAt(tile->id()));
        }
    }
}

/*
void TileLayer::resize(const QSize &size, const QPoint &offset)
{
    // We're currently NOPping this, I think the only part we care about is the offset
}
*/

/*
void TileLayer::offset(const QPoint &offset,
                       const QRect &bounds,
                       bool wrapX, bool wrapY)
{
    QVector<Tile*> newTiles(mWidth * mHeight);

    for (int y = 0; y < mHeight; ++y) {
        for (int x = 0; x < mWidth; ++x) {
            // Skip out of bounds tiles
            if (!bounds.contains(x, y)) {
                newTiles[x + y * mWidth] = tileAt(x, y);
                continue;
            }

            // Get position to pull tile value from
            int oldX = x - offset.x();
            int oldY = y - offset.y();

            // Wrap x value that will be pulled from
            if (wrapX && bounds.width() > 0) {
                while (oldX < bounds.left())
                    oldX += bounds.width();
                while (oldX > bounds.right())
                    oldX -= bounds.width();
            }

            // Wrap y value that will be pulled from
            if (wrapY && bounds.height() > 0) {
                while (oldY < bounds.top())
                    oldY += bounds.height();
                while (oldY > bounds.bottom())
                    oldY -= bounds.height();
            }

            // Set the new tile
            if (contains(oldX, oldY) && bounds.contains(oldX, oldY))
                newTiles[x + y * mWidth] = tileAt(oldX, oldY);
            else
                newTiles[x + y * mWidth] = 0;
        }
    }

    mTiles = newTiles;
}
*/

bool TileLayer::isEmpty() const
{
    for (int y = mSize.top(); y <= mSize.bottom(); ++y)
        for (int x = mSize.left(); x <= mSize.right(); ++x)
            if (tileAt(x, y))
                return false;

    return true;
}

/**
 * Returns a duplicate of this TileLayer.
 *
 * \sa Layer::clone()
 */
Layer *TileLayer::clone() const
{
    return initializeClone(new TileLayer(mName, mX, mY, mSize));  // bleh
}

TileLayer *TileLayer::initializeClone(TileLayer *clone) const
{
    Layer::initializeClone(clone);
    clone->mTiles = mTiles;
    clone->mMaxTileSize = mMaxTileSize;
    return clone;
}
