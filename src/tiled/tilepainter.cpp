/*
 * tilepainter.cpp
 * Copyright 2009-2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "tilepainter.h"

#include "mapdocument.h"
#include "tilelayer.h"
#include "map.h"

#include <QDebug>

using namespace Tiled;
using namespace Tiled::Internal;

// are utility functions meant to be members? god, I don't know
void RefreshMapSizes(MapDocument *map, TileLayer *tilelayer) {
  // may have resized, may not have, so update things.
  // Todo: tell it which map changed, or only emit if it really did resize?
  
  map->map()->setSize(map->map()->size().united(tilelayer->bounds()));
  
  map->emitMapChanged();
}

TilePainter::TilePainter(MapDocument *mapDocument, TileLayer *tileLayer)
    : mMapDocument(mapDocument)
    , mTileLayer(tileLayer)
{
}

Tile *TilePainter::tileAt(int x, int y) const
{
    const int layerX = x - mTileLayer->x();
    const int layerY = y - mTileLayer->y();

    if (!mTileLayer->contains(layerX, layerY))
        return 0;

    return mTileLayer->tileAt(layerX, layerY);
}

void TilePainter::setTile(int x, int y, Tile *tile)
{
    const QRegion &selection = mMapDocument->tileSelection();
    if (!(selection.isEmpty() || selection.contains(QPoint(x, y))))
        return;

    const int layerX = x - mTileLayer->x();
    const int layerY = y - mTileLayer->y();
    
    mTileLayer->setTile(layerX, layerY, tile);
    
    RefreshMapSizes(mMapDocument, mTileLayer);
    
    mMapDocument->emitRegionChanged(QRegion(x, y, 1, 1));
}

void TilePainter::setTiles(int x, int y, TileLayer *tiles, const QRegion &mask)
{
    QRegion region = paintableRegion(x, y, tiles->width(), tiles->height());
    if (!mask.isEmpty())
        region &= mask;
    if (region.isEmpty())
        return;

    foreach (const QRect &rect, region.rects()) {
        for (int _x = rect.left(); _x <= rect.right(); ++_x) {
            for (int _y = rect.top(); _y <= rect.bottom(); ++_y) {
                mTileLayer->setTile(_x - mTileLayer->x(),
                                    _y - mTileLayer->y(),
                                    tiles->tileAt(_x - x, _y - y));
            }
        }
    }

    RefreshMapSizes(mMapDocument, mTileLayer);
    
    mMapDocument->emitRegionChanged(region);
}

void TilePainter::drawTiles(int x, int y, TileLayer *tiles)
{
    const QRegion region = paintableRegion(x, y,
                                           tiles->width(),
                                           tiles->height());
    if (region.isEmpty())
        return;

    foreach (const QRect &rect, region.rects()) {
        for (int _x = rect.left(); _x <= rect.right(); ++_x) {
            for (int _y = rect.top(); _y <= rect.bottom(); ++_y) {
                Tile * const tile = tiles->tileAt(_x - x, _y - y);
                if (!tile)
                    continue;

                mTileLayer->setTile(_x - mTileLayer->x(),
                                    _y - mTileLayer->y(),
                                    tile);
            }
        }
    }
    
    RefreshMapSizes(mMapDocument, mTileLayer);
    
    mMapDocument->emitRegionChanged(region);
}

void TilePainter::drawStamp(const TileLayer *stamp,
                            const QRegion &drawRegion)
{
    Q_ASSERT(stamp);
    if (stamp->bounds().isEmpty())
        return;

    const QRegion region = paintableRegion(drawRegion);
    if (region.isEmpty())
        return;

    const int w = stamp->width();
    const int h = stamp->height();
    const QRect regionBounds = region.boundingRect();

    foreach (const QRect &rect, region.rects()) {
        for (int _x = rect.left(); _x <= rect.right(); ++_x) {
            for (int _y = rect.top(); _y <= rect.bottom(); ++_y) {
                Tile *tile = stamp->tileAt((_x - regionBounds.left()) % w,
                                           (_y - regionBounds.top()) % h);
                if (!tile)
                    continue;

                mTileLayer->setTile(_x - mTileLayer->x(),
                                    _y - mTileLayer->y(),
                                    tile);
            }
        }
    }

    RefreshMapSizes(mMapDocument, mTileLayer);
    
    mMapDocument->emitRegionChanged(region);
}

void TilePainter::erase(const QRegion &region)
{
    const QRegion paintable = paintableRegion(region);
    if (paintable.isEmpty())
        return;

    foreach (const QRect &rect, paintable.rects()) {
        for (int _x = rect.left(); _x <= rect.right(); ++_x) {
            for (int _y = rect.top(); _y <= rect.bottom(); ++_y) {
                mTileLayer->setTile(_x - mTileLayer->x(),
                                    _y - mTileLayer->y(),
                                    0);
            }
        }
    }

    mMapDocument->emitRegionChanged(paintable);
}

QRegion TilePainter::computeFillRegion(const QPoint &fillOrigin) const
{
    // Create that region that will hold the fill
    QRegion fillRegion;

    // Silently quit if parameters are unsatisfactory
    if (!isDrawable(fillOrigin.x(), fillOrigin.y()))
        return fillRegion;

    // Cache tile that we will match other tiles against
    const Tile *matchTile = tileAt(fillOrigin.x(), fillOrigin.y());

    // Grab map dimensions for later use.
    const int mapWidth = mMapDocument->map()->size().width();
    const int mapHeight = mMapDocument->map()->size().height();
    const int mapSize = mapWidth * mapHeight;

    // Create a queue to hold tiles that need filling
    QList<QPoint> fillPositions;
    fillPositions.append(fillOrigin);

    // Create an array that will store which tiles have been processed
    // This is faster than checking if a given tile is in the region/list
    QVector<quint8> processedTilesVec(mapSize);
    quint8 *processedTiles = processedTilesVec.data();

    // Loop through queued positions and fill them, while at the same time
    // checking adjacent positions to see if they should be added
    while (!fillPositions.empty()) {
        const QPoint currentPoint = fillPositions.takeFirst();
        const int startOfLine = currentPoint.y() * mapWidth;

        // Seek as far left as we can
        int left = currentPoint.x();
        while (tileAt(left - 1, currentPoint.y()) == matchTile &&
               isDrawable(left - 1, currentPoint.y()))
            --left;

        // Seek as far right as we can
        int right = currentPoint.x();
        while (tileAt(right + 1, currentPoint.y()) == matchTile &&
               isDrawable(right + 1, currentPoint.y()))
            ++right;

        // Add tiles between left and right to the region
        fillRegion += QRegion(left, currentPoint.y(), right - left + 1, 1);

        // Add tile strip to processed tiles
        memset(&processedTiles[startOfLine + left],
               1,
               right - left);

        // These variables cache whether the last tile was added to the queue
        // or not as an optimization, since adjacent tiles on the x axis
        // do not need to be added to the queue.
        bool lastAboveTile = false;
        bool lastBelowTile = false;

        // Loop between left and right and check if tiles above or
        // below need to be added to the queue
        for (int x = left; x <= right; ++x) {
            const QPoint fillPoint(x, currentPoint.y());

            // Check tile above
            if (fillPoint.y() > 0) {
                QPoint aboveTile(fillPoint.x(), fillPoint.y() - 1);
                if (!processedTiles[aboveTile.y()*mapWidth + aboveTile.x()] &&
                    tileAt(aboveTile.x(), aboveTile.y()) == matchTile &&
                    isDrawable(aboveTile.x(), aboveTile.y()))
                {
                    // Do not add the above tile to the queue if it's
                    // x-adjacent tile was added.
                    if (!lastAboveTile)
                        fillPositions.append(aboveTile);

                    lastAboveTile = true;
                } else lastAboveTile = false;

                processedTiles[aboveTile.y() * mapWidth + aboveTile.x()] = 1;
            }

            // Check tile below
            if (fillPoint.y() + 1 < mapHeight) {
                QPoint belowTile(fillPoint.x(), fillPoint.y() + 1);
                if (!processedTiles[belowTile.y()*mapWidth + belowTile.x()] &&
                    tileAt(belowTile.x(), belowTile.y()) == matchTile &&
                    isDrawable(belowTile.x(), belowTile.y()))
                {
                    // Do not add the below tile to the queue if it's
                    // x-adjacent tile was added.
                    if (!lastBelowTile)
                        fillPositions.append(belowTile);

                    lastBelowTile = true;
                } else lastBelowTile = false;

                processedTiles[belowTile.y() * mapWidth + belowTile.x()] = 1;
            }
        }
    }

    return fillRegion;
}

bool TilePainter::isDrawable(int x, int y) const
{
    const QRegion &selection = mMapDocument->tileSelection();
    if (!(selection.isEmpty() || selection.contains(QPoint(x, y))))
        return false;

    return true;
}

QRegion TilePainter::paintableRegion(const QRegion &region) const
{
    //const QRegion bounds = QRegion(mTileLayer->bounds());
    //QRegion intersection = bounds.intersected(region);
  
    QRegion intersection = region;

    const QRegion &selection = mMapDocument->tileSelection();
    if (!selection.isEmpty())
        intersection &= selection;

    return intersection;
}
