#ifndef DECLARE_H
#define DECLARE_H

enum VorH
{
    Vertical,
    Horizontal
};
bool VorH = Vertical;

enum Mark
{
    unMarked,
    Marked
};

struct tile
{
    int id;             // space tile = 0, solid tile = 1, 2, 3, ...
    int x, y, w, h;     // coordinate of lower-left corner, block width, block height
    tile *top, *bottom; // horizontal pointer: top of right edge, bottom of left edge
    tile *right, *left; // vertical pointer: right of top edge, left of bottom edge
    bool mark;          // unmarked = false, marked = true
};

#endif