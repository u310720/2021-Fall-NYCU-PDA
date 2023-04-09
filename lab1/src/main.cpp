#include <iostream>
#include <fstream>
#include <vector>
#include <string>
using namespace std;
#include "sub.h"

int outlineW, outlineH;
tile *AreaLU; // The tile where the upper left corner of the area is located

int main(int argc, char **argv)
{
    string inFile = argc==1 ? "case3.txt" : argv[1];
    string outFile = "output.txt";

    ifstream ifs;
    ifs.open(inFile);
    ofstream ofs;
    ofs.open(outFile);
    if(1)
    {
        // Read from input.txt
        AreaLU = NewTile();
        int total = 0; // Total solid tiles in the outline
        string id, x, y, w, h;
        ifs >> w >> h;
        outlineW = stoi(w);
        outlineH = stoi(h);
        AreaLU->w = outlineW;
        AreaLU->h = outlineH;

        vector<int> tmpP; // Store the result of the PointFinding
        while (!(ifs >> id).eof())
        {
            if (id == "P")
            {
                ifs >> x >> y;
                if (AreaLU != NULL)
                {
                    tile *t = PointFinding(AreaLU, stoi(x), stoi(y));
                    tmpP.push_back(t->x);
                    tmpP.push_back(t->y);
                    t = NULL;
                }
                else
                    cerr << "No tiles have been created yet";
            }
            else
            {
                ifs >> x >> y >> w >> h;
                total += TileCreation(stoi(id), stoi(x), stoi(y), stoi(w), stoi(h));
            }
        }
        ifs.close();

        // Output to output.txt
        vector<tile *> Enum = TileEnumeration(0, 0, outlineW, outlineH);
        ofs << Enum.size() << endl;
        tile **tmpT = new tile *[total]; // Store the result of sorting the tile's id
        for (int i = 0; i < total; i++)  // Reset tmp[]
            tmpT[i] = NULL;
        int i = 0;
        for (auto it = Enum.begin(); it != Enum.end(); it++, i++) // Sort tile's id
        {
            if (Enum[i]->id != 0)
                tmpT[Enum[i]->id - 1] = Enum[i];
        }
        Enum.clear();
        int solid, space;
        for (int i = 0; i < total; i++) // Count adjacent tiles and output
        {
            CountAdjTiles(tmpT[i], solid, space);
            if (tmpT[i] != NULL)
                ofs << tmpT[i]->id << " " << solid << " " << space << endl;
        }
        for (int i = 0; i < tmpP.size(); i += 2) // Output the PointFinding result
            ofs << tmpP[i] << " " << tmpP[i + 1] << endl;
        tmpP.clear();

        ofs.close();
    }

    return 0;
}