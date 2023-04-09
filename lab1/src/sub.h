#ifndef SUB_H
#define SUB_H

#include <iostream>
#include <vector>
using namespace std;
#include "declare.h"

extern tile *AreaLU;
extern int outlineH;

tile *NewTile();                                                                                               // malloc and reset it
void SetTile(tile *cur, int id, int x, int y, int w, int h, tile *top, tile *bottom, tile *left, tile *right); // Set the attributes of the tile

tile *PointFinding(tile *cur, int x, int y); // Returns the tile where the point is located

bool TileCreation(int id, int x, int y, int w, int h);                // Insert a block in the outline, return 1 on successful creation, 0 on failure
void Split_Top_Bottom(int x, int y, int width, int height);           // Split the top and bottom sides
void Split_left_right(tile *cur, int id, int x, int y, int w, int h); // Split the left and right sides and merge them by function MergeTile()
void AdjacentPtrFix(tile *cur);                                       // Repair the pointer of the adjacent tile
void MergeTile(tile *cur);                                            // Try to merge cur, cur->top and cur->bottom

bool AreaSearch(int x, int y, int w, int h); // If there are solid tile in the area, return true otherwise return false

vector<tile *> TileEnumeration(int x, int y, int w, int h);                // Enumerate all tiles in the area(x, y, w, h)
void EnumDFS(tile *cur, vector<tile *> &Enum, int x, int y, int w, int h); // A step of TileEnumeration

void CountAdjTiles(tile *cur, int &solid, int &space); // Find out how many space tiles and solid tiles around current tile

tile *NewTile()
{
    tile *t = new tile;
    t->id = 0;
    t->x = 0;
    t->y = 0;
    t->w = 0;
    t->h = 0;
    t->top = nullptr;
    t->bottom = nullptr;
    t->left = nullptr;
    t->right = nullptr;
    t->mark = unMarked;
    return t;
}
void SetTile(tile *cur, int id, int x, int y, int w, int h, tile *top, tile *bottom, tile *left, tile *right)
{
    cur->id = id;
    cur->x = x;
    cur->y = y;
    cur->w = w;
    cur->h = h;
    cur->top = top;
    cur->bottom = bottom;
    cur->left = left;
    cur->right = right;
}

tile *PointFinding(tile *cur, int x, int y)
{
    if (cur->x <= x && x < cur->x + cur->w && cur->y < y && y <= cur->y + cur->h)
    {
        VorH = Vertical;
        return cur;
    }

    switch (VorH)
    {
    case Vertical:
        if (cur->y < y && y <= cur->y + cur->h)
        {
            VorH = Horizontal;
            return PointFinding(cur, x, y);
        }
        else if (cur->y >= y)
        {
            if (cur->bottom != nullptr)
                return PointFinding(cur->bottom, x, y);
            else
                return nullptr;
        }
        else
        {
            if (cur->top != nullptr)
                return PointFinding(cur->top, x, y);
            else
                return nullptr;
        }
        break;
    case Horizontal:
        if (cur->x <= x && x < cur->x + cur->w)
        {
            VorH = Vertical;
            return PointFinding(cur, x, y);
        }
        else if (cur->x > x)
        {
            if (cur->left != nullptr)
                return PointFinding(cur->left, x, y);
            else
                return nullptr;
        }
        else
        {
            if (cur->right != nullptr)
                return PointFinding(cur->right, x, y);
            else
                return nullptr;
        }
        break;
    }
}

bool AreaSearch(int x, int y, int w, int h)
{
    AreaLU = PointFinding(AreaLU, x, y + h);
    tile *cur = AreaLU;
    if (cur->id != 0 || cur->x + cur->w < x + w) // Current tile is solid or the right edge of current tile is in the area
        return true;
    while (cur->y > y)
    {
        cur = PointFinding(cur, x, cur->y);
        if (cur->id != 0 || cur->x + cur->w < x + w) // Current tile is solid or the right edge of current tile is in the area
            return true;
    }
    return false;
}

void EnumDFS(tile *cur, vector<tile *> &Enum, int x, int y, int w, int h)
{
    Enum.push_back(cur);
    cur->mark = Marked;
    tile *t = cur->right;
    while (t != nullptr && t->y >= cur->y)
    {
        if (t != nullptr)
            if (t->mark == unMarked)                                                         // Not yet visited
                if (t->x < x + w && t->x >= x && t->y < y + h && t->y >= y                   // The lower left corner of t is in the area
                    || t->x < x + w && t->x >= x && t->y + t->h > y && t->y + t->h <= y + h) // The upper left corner of t is in the area
                    EnumDFS(t, Enum, x, y, w, h);
        t = t->bottom;
    }
}
vector<tile *> TileEnumeration(int x, int y, int w, int h)
{
    vector<tile *> Enum;
    vector<tile *> L; // Tile on the left edge
    AreaLU = PointFinding(AreaLU, x, y + h);
    tile *cur = AreaLU;
    L.push_back(cur);
    while (cur->y > y)
    {
        cur = PointFinding(cur, x, cur->y);
        L.push_back(cur);
    }
    int i = 0;
    for (auto it = L.begin(); it != L.end(); it++, i++)
        EnumDFS(L[i], Enum, x, y, w, h);
    i = 0;
    for (auto it = Enum.begin(); it != Enum.end(); it++, i++)
        Enum[i]->mark = unMarked; // reset mark
    return Enum;
}

bool TileCreation(int id, int x, int y, int w, int h)
{
    // Step 0: Find out if there is a solid tile in the range, stop execution if found
    if (AreaSearch(x, y, w, h))
    {
        cerr << "Overlap! Unable to create block " << id << endl;
        return false;
    }

    // Step 1: Split the top and bottom sides
    Split_Top_Bottom(x, y, w, h);

    // Step 2: Split the left and right sides and merge them
    vector<tile *> Enum = TileEnumeration(x, y, w, h);
    int i = 0;
    for (auto it = Enum.begin(); it != Enum.end(); it++, i++)
    {
        Split_left_right(Enum[i], id, x, y, w, h);
        Enum[i] = nullptr;
    }
    return true;
}
void Split_Top_Bottom(int x, int y, int w, int h)
{
    if (y != 0) // If y==0, there is no need to split the bottom
    {
        tile *bot0 = PointFinding(AreaLU, x, y); // Find which tile is the lower left corner
        if (y != bot0->y + bot0->h)              // The point is not on the edge of the tile
        {
            tile *bot1 = NewTile();
            SetTile(bot1, 0, bot0->x, y, bot0->w, bot0->y + bot0->h - y, bot0->top, bot0, PointFinding(bot0, bot0->x - 1, y + 1), bot0->right); // set top half
            SetTile(bot0, 0, bot0->x, bot0->y, bot0->w, y - bot0->y, bot1, bot0->bottom, bot0->left, PointFinding(bot0, bot0->x + bot0->w, y)); // shrink bot0
            AdjacentPtrFix(bot0);
            AdjacentPtrFix(bot1);
            bot0 = nullptr;
            bot1 = nullptr;
        }
    }
    if (y != outlineH) // If y==outlineH, there is no need to split the top
    {
        tile *top0 = PointFinding(AreaLU, x, y + h); // Find which tile is the upper left corner
        if (y + h != top0->y + top0->h)              // The point is not on the edge of the tile
        {
            tile *top1 = NewTile();
            SetTile(top1, 0, top0->x, y + h, top0->w, top0->y + top0->h - y - h, top0->top, top0, top0->left, top0->right);                             // set top half
            SetTile(top0, 0, top0->x, top0->y, top0->w, y + h - top0->y, top1, top0->bottom, top0->left, PointFinding(top0, top0->x + top0->w, y + h)); // shrink top0
            AdjacentPtrFix(top0);
            AdjacentPtrFix(top1);
            top0 = nullptr;
            top1 = nullptr;
        }
    }
}
void AdjacentPtrFix(tile *cur)
{
    tile *t = cur->top;
    while (t != nullptr && t->x >= cur->x)
    {
        t->bottom = cur;
        t = t->left;
    }
    t = cur->left;
    while (t != nullptr && t->y + t->h <= cur->y + cur->h)
    {
        t->right = cur;
        t = t->top;
    }
    t = cur->right;
    while (t != nullptr && t->y >= cur->y)
    {
        t->left = cur;
        t = t->bottom;
    }
    t = cur->bottom;
    while (t != nullptr && t->x + t->w <= cur->x + cur->w)
    {
        t->top = cur;
        t = t->right;
    }
    t = nullptr;
}
void Split_left_right(tile *cur, int id, int x, int y, int w, int h)
{
    if (cur->x == x && cur->w == w) // 0-S-0
    {
        cur->id = id;
        cur->top = PointFinding(cur, cur->x + cur->w - 1, cur->y + cur->h + 1);
        MergeTile(cur);
        cur = nullptr;
    }
    else if (cur->x < x && cur->x + cur->w == x + w) // L-S-0
    {
        tile *l = cur;
        tile *s = NewTile();
        SetTile(s, id, x, cur->y, w, cur->h, PointFinding(cur, cur->x + cur->w - 1, cur->y + cur->h + 1), PointFinding(cur, x, cur->y), l, cur->right);
        l->top = PointFinding(cur, x - 1, cur->y + cur->h + 1);
        l->right = s;
        l->w = x - cur->x;
        AdjacentPtrFix(l);
        AdjacentPtrFix(s);
        MergeTile(l);
        MergeTile(s);
        l = nullptr;
        s = nullptr;
        cur = nullptr;
    }
    else if (cur->x == x && cur->w > w) // 0-S-R
    {
        tile *r = cur;
        tile *s = NewTile();
        SetTile(s, id, x, cur->y, w, cur->h, PointFinding(cur, x + w - 1, cur->y + cur->h + 1), PointFinding(cur, x, cur->y), cur->left, r);
        r->bottom = PointFinding(cur, x + w, cur->y);
        r->left = s;
        r->x = x + w;
        r->w = cur->w - w;
        AdjacentPtrFix(s);
        AdjacentPtrFix(r);
        MergeTile(s);
        MergeTile(r);
        s = nullptr;
        r = nullptr;
        cur = nullptr;
    }
    else // L-S-R, case: (cur->x < x && cur->x + cur->w > x + w)
    {
        tile *l = cur;
        tile *s = NewTile();
        tile *r = NewTile();
        SetTile(r, 0, x + w, cur->y, cur->x + cur->w - x - w, cur->h, cur->top, PointFinding(cur, x + w, cur->y), s, cur->right);
        SetTile(s, id, x, cur->y, w, cur->h, PointFinding(cur, x + w - 1, cur->y + cur->h + 1), PointFinding(cur, x, cur->y), l, r);
        l->top = PointFinding(cur, x - 1, cur->y + cur->h + 1);
        l->right = s;
        l->w = x - l->x;
        AdjacentPtrFix(l);
        AdjacentPtrFix(s);
        AdjacentPtrFix(r);
        MergeTile(l);
        MergeTile(s);
        MergeTile(r);
        l = nullptr;
        s = nullptr;
        r = nullptr;
        cur = nullptr;
    }
}
void MergeTile(tile *cur)
{
    if (cur->top != nullptr)
    {
        if (cur->x == cur->top->x && cur->w == cur->top->w && cur->id == cur->top->id) // cur and cur->top have the same x, w and id
        {
            // Step 1: Fix neighbor's pointer
            tile *t = cur->top->top;
            while (t != nullptr && t->x >= cur->top->x)
            {
                t->bottom = cur;
                t = t->left;
            }
            t = cur->top->left;
            while (t != nullptr && t->y + t->h <= cur->top->y + cur->top->h)
            {
                t->right = cur;
                t = t->top;
            }
            t = cur->top->right;
            while (t != nullptr && t->y >= cur->top->y)
            {
                t->left = cur;
                t = t->bottom;
            }
            // Step 2: Expansion cur and fix cur's pointer, then delete cur->top
            t = cur->top;
            cur->right = t->right;
            cur->top = t->top;
            cur->h = cur->h + t->h;
            if(AreaLU == t)
                AreaLU = cur;
            delete t;
            t = nullptr;
        }
    }
    if (cur->bottom != nullptr) // Try to merge the last tile in the vector, check if it can be combined with cur->bottom
    {
        if (cur->x == cur->bottom->x && cur->w == cur->bottom->w && cur->id == 0 && cur->top->id == 0) // Current tile and current->bottom have the same x and w and both are space tiles
        {
            // Step 1: Fix neighbor's pointer
            tile *t = cur->bottom->bottom;
            while (t != nullptr && t->x + t->w <= cur->bottom->x + cur->bottom->w)
            {
                t->top = cur;
                t = t->right;
            }
            t = cur->bottom->left;
            while (t != nullptr && t->y + t->h <= cur->bottom->y + cur->bottom->h)
            {
                t->right = cur;
                t = t->top;
            }
            t = cur->bottom->right;
            while (t != nullptr && t->y >= cur->bottom->y)
            {
                t->left = cur;
                t = t->bottom;
            }
            // Step 2: Expansion cur and fix cur's pointer, then delete cur->top
            t = cur->bottom;
            cur->left = t->left;
            cur->bottom = t->bottom;
            cur->h = cur->h + t->h;
            cur->y = t->y;
            if(AreaLU == t)
                AreaLU = cur;
            delete t;
            t = nullptr;
        }
    }
}

void CountAdjTiles(tile *cur, int &solid, int &space)
{
    solid = 0;
    space = 0;
    tile *t = cur->top;
    while (t != nullptr && t->x + t->w > cur->x)
    {
        if (t->id == 0)
            space++;
        else
            solid++;
        t = t->left;
    }
    t = cur->left;
    while (t != nullptr && t->y < cur->y + cur->h)
    {
        if (t->id == 0)
            space++;
        else
            solid++;
        t = t->top;
    }
    t = cur->right;
    while (t != nullptr && t->y + t->h > cur->y)
    {
        if (t->id == 0)
            space++;
        else
            solid++;
        t = t->bottom;
    }
    t = cur->bottom;
    while (t != nullptr && t->x < cur->x + cur->w)
    {
        if (t->id == 0)
            space++;
        else
            solid++;
        t = t->right;
    }
    t = nullptr;
}

#endif