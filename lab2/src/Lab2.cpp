#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <ctime>
using namespace std;

const pair<double, double> BalenceFactor = make_pair(0.45, 0.55);

struct cell
{
    int id, gain;
    set<int> connectedNet; // the id of which networks this cell is connected
    bool moved, updated, part;
};

struct Data
{
    vector<cell *> CellList;
    vector<vector<cell *>> Net;
    pair<map<int, list<cell *>>, map<int, list<cell *>>> GainBucket;
    vector<cell *> CutList;
    pair<int, int> AreaConstraint;
    int P0_Size = 0, P1_Size = 0;
};

void OutputPartition(vector<cell *> &CellList)
{
    ofstream ofs;
    ofs.open("output.txt");
    if (!ofs)
    {
        cerr << "File could not open!" << endl;
        return;
    }
    for (auto it = CellList.begin() + 1; it != CellList.end(); it++)
        ofs << (*it)->part << endl;
    ofs.close();
}

int CalcGain(cell *n, Data *Dptr)
{
    int FS = 0, TE = 0;
    for (const auto &i : n->connectedNet)
    {
        int sameCount = 0, differentCount = 0;
        for (const auto &it : Dptr->Net[i])
        {
            if (it->part != n->part) // Two cells on different parts
                differentCount++;
            else if (it != n) // Two different cells on the same parts
                sameCount++;
            if (differentCount > 0 && sameCount > 0)
                break;
        }
        TE = differentCount == 0 ? TE + 1 : TE;
        FS = sameCount == 0 ? FS + 1 : FS;
    }
    return FS - TE;
}

struct initial
{
    struct Data *Dptr;

    void halfPartition(int n)
    {
        int half = n / 2;
        srand(time(NULL));
        Dptr->CellList.push_back(nullptr);
        for (int i = 0; i < n; i++)
        {
            cell *tmp = new cell;
            tmp->id = i + 1;
            tmp->gain = 0;
            tmp->moved = false;
            tmp->updated = false;
            if (i < half)
            {
                tmp->part = 0;
                Dptr->P0_Size++;
            }
            else
            {
                tmp->part = 1;
                Dptr->P1_Size++;
            }
            Dptr->CellList.push_back(tmp);
        }
    }
    int initialPartition(string &fin)
    {
        ifstream ifs;
        ifs.open(fin);
        if (!ifs)
        {
            cerr << "File could not open!" << endl;
            return 1;
        }
        string tmp;
        ifs >> tmp; // number of nets
        ifs >> tmp; // number of cells
        int CellNum = stoi(tmp);
        Dptr->AreaConstraint.first = CellNum * BalenceFactor.first + 1;
        Dptr->AreaConstraint.second = CellNum * BalenceFactor.second;
        halfPartition(CellNum);
        int NetNum = 0;
        while (!ifs.eof())
        {
            vector<cell *> subNetTmp;
            getline(ifs, tmp);
            istringstream f(tmp);
            string s;
            while (getline(f, s, ' '))
            {
                Dptr->CellList[stoi(s)]->connectedNet.insert(NetNum);
                subNetTmp.push_back(Dptr->CellList[stoi(s)]);
            }
            Dptr->Net.push_back(subNetTmp);
            NetNum++;
        }
        ifs.close();
        Dptr->Net.pop_back();
        return 0;
    }

    int findPmax()
    {
        int Pmax = 0;
        for (auto it = Dptr->CellList.begin() + 1; it != Dptr->CellList.end(); it++)
            Pmax = Pmax < (int)(*it)->connectedNet.size() ? (*it)->connectedNet.size() : Pmax;
        return Pmax;
    }
    void CreateGainBucket()
    {
        int Pmax = findPmax();
        for (int i = -Pmax; i <= Pmax; i++)
        {
            list<cell *> tmp;
            Dptr->GainBucket.first.insert(make_pair(i, tmp));
            Dptr->GainBucket.second.insert(make_pair(i, tmp));
        }
    }
};

struct FM
{
    struct Data *Dptr;

    void FillGainBucket(vector<cell *> v)
    {
        auto it = *v.begin() == nullptr ? v.begin() + 1 : v.begin();
        for (; it != v.end(); it++) // put the cell in the bucket
        {
            (*it)->moved = false;
            (*it)->gain = CalcGain(*it, Dptr);
            if ((*it)->part)
                Dptr->GainBucket.second[(*it)->gain].push_back(*it);
            else
                Dptr->GainBucket.first[(*it)->gain].push_back(*it);
        }
    }
    bool BucketNotEmpty()
    {
        for (const auto &it : Dptr->GainBucket.first)
            if (!it.second.empty())
                return true;
        for (const auto &it : Dptr->GainBucket.second)
            if (!it.second.empty())
                return true;
        return false;
    }
    int Cutsize()
    {
        int Cutsize = 0;
        for (auto itN = Dptr->Net.begin() + 1; itN != Dptr->Net.end(); itN++)
        {
            bool part = itN->front()->part;
            for (const auto &it : *itN)
                if (part != it->part)
                {
                    Cutsize++;
                    break;
                }
        }
        return Cutsize;
    }
    void UpdateGain(cell *n)
    {
        if (n->part)
        {
            auto it = Dptr->GainBucket.second[n->gain].begin();
            for (; it != Dptr->GainBucket.second[n->gain].end(); it++)
                if (*it == n) // find n
                    break;
            Dptr->GainBucket.second[n->gain].erase(it);
            n->gain = CalcGain(n, Dptr);
            Dptr->GainBucket.second[n->gain].push_back(n);
        }
        else
        {
            auto it = Dptr->GainBucket.first[n->gain].begin();
            for (; it != Dptr->GainBucket.first[n->gain].end(); it++)
                if (*it == n) // find n
                    break;
            Dptr->GainBucket.first[n->gain].erase(it);
            n->gain = CalcGain(n, Dptr);
            Dptr->GainBucket.first[n->gain].push_back(n);
        }
    }
    int MoveCell(cell *n)
    {
        if (n->part)
        {
            auto it = Dptr->GainBucket.second[n->gain].begin();
            for (; it != Dptr->GainBucket.second[n->gain].end(); it++)
                if (*it == n) // find n
                    break;
            Dptr->GainBucket.second[n->gain].erase(it);
        }
        else
        {
            auto it = Dptr->GainBucket.first[n->gain].begin();
            for (; it != Dptr->GainBucket.first[n->gain].end(); it++)
                if (*it == n) // find n
                    break;
            Dptr->GainBucket.first[n->gain].erase(it);
        }
        n->part = !n->part; // move to another side
        n->moved = true;
        list<cell *> updated;
        for (const auto &i : n->connectedNet) // update neighbor's gain
            for (const auto &itN : Dptr->Net[i])
                if (!itN->moved && !itN->updated) // Not update yet
                {
                    UpdateGain(itN);
                    itN->updated = true;
                    updated.push_front(itN);
                }
        for (auto &it : updated)
            it->updated = false;
        return n->gain;
    }
    cell *FindMaxGainCell(map<int, list<cell *>> &Bucket)
    {
        for (auto it = Bucket.rbegin(); it != Bucket.rend(); it++) // Bucket[Pmax] to Bucket[-Pmax]
            if (!it->second.empty())
                return it->second.front();
        return nullptr;
    }
    pair<int, cell *> FMStep() // Choose a suitable cell to move
    {
        cell *c0, *c1;
        if (Dptr->P0_Size - 1 >= Dptr->AreaConstraint.first)
            c0 = FindMaxGainCell(Dptr->GainBucket.first);
        else
            c0 = nullptr;
        if (Dptr->P1_Size - 1 >= Dptr->AreaConstraint.first)
            c1 = FindMaxGainCell(Dptr->GainBucket.second);
        else
            c1 = nullptr;
        if (c0 == nullptr)
        {
            Dptr->P1_Size--;
            Dptr->P0_Size++;
            return make_pair(MoveCell(c1), c1); // return gain(cell)
        }
        else if (c1 == nullptr)
        {
            Dptr->P0_Size--;
            Dptr->P1_Size++;
            return make_pair(MoveCell(c0), c0); // return gain(cell)
        }
        else if (c0->gain > c1->gain)
        {
            Dptr->P0_Size--;
            Dptr->P1_Size++;
            return make_pair(MoveCell(c0), c0); // return gain(cell)
        }
        else if(c0->gain == c1->gain)
        {
            if(Dptr->P0_Size > Dptr->P1_Size)
            {
                Dptr->P0_Size--;
                Dptr->P1_Size++;
                return make_pair(MoveCell(c0), c0); // return gain(cell)
            }
            else
            {
                Dptr->P0_Size++;
                Dptr->P1_Size--;
                return make_pair(MoveCell(c1), c1); // return gain(cell)
            }
        }
        else
        {
            Dptr->P1_Size--;
            Dptr->P0_Size++;
            return make_pair(MoveCell(c1), c1); // return gain(cell)
        }
    }
    int FMPass() // FM_main
    {
        int MaxGain = 0;
        pair<int, cell *> Gain_MovedCell = make_pair(1, nullptr);
        while (BucketNotEmpty() && Gain_MovedCell.first >= 0) // find local optimimum
        {
            if(clock() / CLOCKS_PER_SEC > 25)
                break;
            Gain_MovedCell = FMStep();
            MaxGain = MaxGain < Gain_MovedCell.first ? Gain_MovedCell.first : MaxGain;
            Dptr->CutList.push_back(Gain_MovedCell.second);
        }
        if (!Dptr->CutList.empty())
        {
            if (Dptr->CutList.back()->part)
            {
                Dptr->CutList.back()->part = !Dptr->CutList.back()->part;
                Dptr->P1_Size--;
                Dptr->P0_Size++;
            }
            else
            {
                Dptr->CutList.back()->part = !Dptr->CutList.back()->part;
                Dptr->P1_Size++;
                Dptr->P0_Size--;
            }
        }
        return MaxGain;
    }
};

int main(int argc, char **argv)
{
    string fin = argc == 1 ? "input2.hgr" : argv[1];
    Data D;
    FM fm = {&D};
    initial init = {&D};
    if (init.initialPartition(fin))
        return 0;

    init.CreateGainBucket();

    int pass = 0;
    do
    {
        if (pass == 0)
            fm.FillGainBucket(D.CellList);
        else
            fm.FillGainBucket(D.CutList); // store which cell moved in a pass
        D.CutList.clear();
        pass++;
    } while (fm.FMPass() > 0);

    // cout << "Cutsize: " << fm.Cutsize() << endl; // debug
    OutputPartition(D.CellList);

    //cout << "Execution time: " << clock() / CLOCKS_PER_SEC << "sec" << endl;

    return 0;
}