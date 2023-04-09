// Maybe try inserting the track in place and maze_routing() again to fix the failed routing(i.e. no path exist)
// router.extractMinDistPoint() and router.BFS() can be optimized using heap instead
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <deque>
#include <algorithm>
#include <climits>
using namespace std;
// #define DEBUG

#ifdef DEBUG
#include <iomanip>   // for debug
#include <conio.h>   // for debug
#include <windows.h> // for debug
#endif

#define point pair<int, int>
#define X first
#define Y second
#define track vector<grid>

enum grid_state
{
    Pass,
    VPass,
    HPass,
    NoPass
};
enum Net_state
{
    V,
    H,
    Via,
    Pin
};
enum direction
{
    // up, down, left, right
    U,
    D,
    L,
    R
};
struct grid
{
    int dist; // distance
    int state;
    bool visited;
    map<int, int> net_state; // netID & passing direction

    grid() : dist(-1) {}
    bool existNet(int netID) { return net_state.find(netID) != net_state.end(); }
    int netState(int netID) { return net_state.find(netID)->second; }
    grid copyGrid(grid *upGrid) // for addTrack()
    {
        grid g;
        if (state == Pass || state == VPass)
            g.state = Pass;
        else if (state == HPass)
        {
            g.state = HPass;
            g.net_state = net_state;
        }
        else // state == NoPass
        {
            if (net_state.size() == 1) // this is a via
            {
                if (upGrid->existNet(net_state.begin()->first))
                {
                    g.state = HPass;
                    g.net_state.insert(make_pair(net_state.begin()->first, V));
                }
                else
                    g.state = Pass;
            }
            else // 2 nets intersect in this grid
            {
                g.state = HPass;
                if (net_state.begin()->second == V)
                    g.net_state.insert(*net_state.begin());
                else
                    g.net_state.insert(*net_state.rbegin());
            }
        }
        return g;
    }
    bool canPass(int direction) // for maze_routing()
    {
        if (state == Pass ||
            (direction == U || direction == D) && state == VPass ||
            (direction == L || direction == R) && state == HPass)
            return true;
        else
            return false;
    }
};
struct segment // horizontal segment
{
    int color; // for findCycle()
    int netID, LX, RX, indegree;
    set<segment *> out;

    segment(int _netID, int _LX, int _RX) : netID(_netID), LX(_LX), RX(_RX), indegree(0) {}
    bool operator<(const segment &s)
    {
        if (indegree == s.indegree)
            return LX < s.LX;
        else
            return indegree < s.indegree;
    }
};

struct debug
{
    vector<track> *T;
    ofstream ofs;
    int *width, length;
    void initial(vector<track> *_T, int *_width, int _length)
    {
#ifdef DEBUG
        T = _T;
        width = _width;
        length = _length;
        ofs.open("debug.txt");
        for (int i = 0; i < length; i++)
            ofs << setw(4) << left << i;
        ofs << endl
            << endl;
#endif
    }
    void printTracks()
    {
#ifdef DEBUG
        int i = *width - 1;
        for (const auto &it : (*T)[i])
            ofs << left << setw(4) << it.net_state.begin()->first;
        ofs << endl;
        for (i = i - 1; i > 0; i--)
        {
            for (const auto &it : (*T)[i])
                if (it.state == Pass)
                    ofs << left << setw(4) << "";
                else if (it.state == HPass)
                    ofs << left << setw(4) << "|";
                else if (it.state == VPass)
                    ofs << left << setw(4) << "-";
                else
                    ofs << left << setw(4) << "+";
            ofs << endl;
        }
        for (const auto &it : (*T)[0])
            ofs << left << setw(4) << it.net_state.begin()->first;
        ofs << endl
            << endl;
#endif
    }
    void printNets()
    {
#ifdef DEBUG
        for (int i = *width - 1; i >= 0; i--)
        {
            for (int j = 0; j < length; j++)
            {
                if ((*T)[i][j].net_state.empty())
                    ofs << left << setw(4) << "";
                else
                    ofs << left << setw(4) << (*T)[i][j].net_state.begin()->first;
            }
            ofs << "   ";
            for (int j = 0; j < length; j++)
            {
                if ((*T)[i][j].net_state.size() == 2)
                    ofs << left << setw(4) << (*T)[i][j].net_state.rbegin()->first;
                else
                    ofs << left << setw(4) << ".";
            }
            ofs << endl;
        }
        ofs << endl
            << endl;
#endif
    }
    string netStateConverter(int state)
    {
#ifdef DEBUG
        switch (state)
        {
        case V:
            return "V";
            break;
        case H:
            return "H";
            break;
        case Via:
            return "Via";
            break;
        case Pin:
            return "Pin";
            break;
        default:
            return "Error";
            break;
        }
#else
        return "";
#endif
    }
    void printNetState()
    {
#ifdef DEBUG
        for (int i = *width - 1; i >= 0; i--)
        {
            for (int j = 0; j < length; j++)
            {
                if ((*T)[i][j].net_state.empty())
                    ofs << left << setw(4) << "";
                else
                    ofs << left << setw(4) << netStateConverter((*T)[i][j].net_state.begin()->second);
            }
            ofs << "   ";
            for (int j = 0; j < length; j++)
            {
                if ((*T)[i][j].net_state.size() == 2)
                    ofs << left << setw(4) << netStateConverter((*T)[i][j].net_state.rbegin()->second);
                else
                    ofs << left << setw(4) << ".";
            }
            ofs << endl;
        }
        ofs << endl
            << endl;
#endif
    }
    void printSegment(segment *s)
    {
#ifdef DEBUG
        ofs << "****** " << s->netID << "[" << s->LX << ", " << s->RX << "] ******" << endl; // for debug
#endif
    }
    void printAddTrack()
    {
#ifdef DEBUG
        ofs << "************* addTracks() ***************" << endl;
#endif
    }
    void setColor(int color)
    {
#ifdef DEBUG
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#endif
    }
    void printDist()
    {
#ifdef DEBUG
        ofs << endl
            << endl;
        for (int i = *width - 1; i >= 0; i--)
        {
            for (int j = 0; j < length; j++)
                if ((*T)[i][j].dist == INT_MAX)
                    ofs << left << setw(4) << ".";
                else
                    ofs << left << setw(4) << (*T)[i][j].dist;
            ofs << endl;
        }
#endif
    }
    void printDistOnTerminal(int change_x, int change_y)
    {
#ifdef DEBUG
        cout << endl
             << endl;
        for (int i = *width - 1; i >= 0; i--)
        {
            for (int j = 0; j < length; j++)
                if ((*T)[i][j].dist == INT_MAX)
                    cout << left << setw(4) << ".";
                else if (i == change_y && j == change_x)
                {
                    setColor(240);
                    cout << left << setw(4) << (*T)[i][j].dist;
                    setColor(15);
                }
                else
                    cout << left << setw(4) << (*T)[i][j].dist;
            cout << endl;
        }
#endif
    }
} dbg;

struct graph
{
    int length, nSegment;
    vector<int> *TPin, *BPin;
    map<int, int> *nets;
    deque<segment> S;

    enum mark
    {
        white,
        gray,
        black
    };
    void initial(int _length, vector<int> *_TPin, vector<int> *_BPin, map<int, int> *_nets)
    {
        length = _length;
        TPin = _TPin;
        BPin = _BPin;
        nets = _nets;
    }
    void createSegment()
    {
        map<int, vector<segment *>> net_seg;
        for (auto &itn : *nets)
        {
            int i, netID = itn.first;
            for (i = 0; i < length; i++)
                if (TPin->at(i) == netID || BPin->at(i) == netID)
                    break;
            int begin = i;
            for (i = i + 1; i < length; i++)
            {
                if (TPin->at(i) == netID || BPin->at(i) == netID)
                {
                    segment s(netID, begin, i);
                    begin = i;
                    S.push_back(s);
                }
            }
        }
    }
    void buildGraph()
    {
        createSegment();
        nSegment = S.size();
        sort(S.begin(), S.end());

        // set vertical constraint
        for (int x = 0; x < length; x++)
        {
            if (TPin->at(x) == 0 || BPin->at(x) == 0)
                continue;
            else if (TPin->at(x) != BPin->at(x))
            {
                vector<segment *> ST, SB;
                for (auto &it : S)
                    if (it.netID == TPin->at(x))
                        if (it.LX == x || it.RX == x)
                            ST.push_back(&it);
                for (auto &it : S)
                    if (it.netID == BPin->at(x))
                        if (it.LX == x || it.RX == x)
                            SB.push_back(&it);
                for (auto &sb : SB)
                    for (auto &st : ST)
                    {
                        if (sb->out.find(st) == sb->out.end())
                        {
                            st->indegree++;
                            sb->out.insert(st);
                        }
                    }
            }
        }
    }
    pair<segment *, segment *> DFS(segment *s)
    {
        static segment *begin, *end;
        static bool findCycle = false;

        s->color = gray;
        for (auto &it : s->out)
        {
            if (it->color == white)
                tie(begin, end) = DFS(it);
            else if (it->color == gray)
            {
                findCycle = true;
                begin = it;
                end = s;
            }

            if (findCycle)
                return make_pair(begin, end);
        }

        dbg.printSegment(s); // for debug
        s->color = black;
        return make_pair(nullptr, nullptr);
    }
    pair<segment *, segment *> findCycle(deque<segment *> &_S)
    {
        for (auto &it : _S)
            it->color = white;

        segment pseudo_source(-1, -1, -1);
        for (auto &it : S)
            if (it.indegree == 1)
                pseudo_source.out.insert(&it);

        segment *begin, *end;
        tie(begin, end) = DFS(&pseudo_source);

#ifdef DEBUG // Normally a circle will be found
        if (begin == nullptr && end == nullptr)
            cout << "no cycle." << endl;
#endif

        return make_pair(begin, end);
    }
};
struct router
{
    // constrained left edge + maze routing
    int length, *width;
    vector<int> *TPin, *BPin;
    vector<track> *T;
    graph *VCG;

    void initial(int _length, int *_width, vector<int> *_TPin, vector<int> *_BPin, vector<track> *_T, graph *_VCG)
    {
        length = _length;
        width = _width;
        TPin = _TPin;
        BPin = _BPin;
        T = _T;
        VCG = _VCG;
    }
    bool tryAddSegment(segment *s) // for tryLeft_edge()
    {
        // vertical wire
        if (T->at(0)[s->LX].existNet(s->netID))
        {
            vector<grid *> tmp;
            for (auto it = T->end() - 3; it != T->begin(); it--) // start from first track below this segment
                if (it->at(s->LX).state == NoPass && !it->at(s->LX).existNet(s->netID))
                    return EXIT_FAILURE;
            for (auto it = T->end() - 3; !it->at(s->LX).existNet(s->netID); it--) // find grids containing the same net below this segments
                tmp.push_back(&it->at(s->LX));
            for (const auto &it : tmp)
            {
                it->net_state.insert(make_pair(s->netID, V));
                if (it->state == VPass)
                    it->state = NoPass;
                else // it->state == Pass
                    it->state = HPass;
            }
        }
        if (T->at(0)[s->RX].existNet(s->netID))
        {
            vector<grid *> tmp;
            for (auto it = T->end() - 3; it != T->begin(); it--) // start from first track below this segment
                if (it->at(s->RX).state == NoPass && !it->at(s->RX).existNet(s->netID))
                    return EXIT_FAILURE;
            for (auto it = T->end() - 3; !it->at(s->RX).existNet(s->netID); it--) // find grids containing the same net below this segments
                tmp.push_back(&it->at(s->RX));
            for (const auto &it : tmp)
            {
                it->net_state.insert(make_pair(s->netID, V));
                if (it->state == VPass)
                    it->state = NoPass;
                else // it->state == Pass
                    it->state = HPass;
            }
        }

        // endpoints
        auto *Tptr = &T->at(*width - 2);
        Tptr->at(s->LX).state = NoPass;
        Tptr->at(s->RX).state = NoPass;
        if (Tptr->at(s->LX).existNet(s->netID))
            Tptr->at(s->LX).net_state.find(s->netID)->second = Via;
        else
            Tptr->at(s->LX).net_state.insert(make_pair(s->netID, Via));
        if (Tptr->at(s->RX).existNet(s->netID))
            Tptr->at(s->RX).net_state.find(s->netID)->second = Via;
        else
            Tptr->at(s->RX).net_state.insert(make_pair(s->netID, Via));

        // horizontal wire
        for (int i = s->LX + 1; i < s->RX; i++)
        {
            Tptr->at(i).net_state.insert(make_pair(s->netID, H));

            if (Tptr->at(i).state == Pass)
                Tptr->at(i).state = VPass;
            else
            {
                if (Tptr->at(i).state == HPass)
                    Tptr->at(i).state = NoPass;
                else
                    Tptr->at(i).state = VPass;
            }
        }
        return EXIT_SUCCESS;
    }
    void columnConnect()
    {
        // excute after initialization
        // if the pins of the same column belong to the same net, then connect them
        for (int i = 0; i < length; i++)
            if (TPin->at(i) == BPin->at(i) && TPin->at(i) != 0)
            {
                T->at(1)[i].net_state.insert(make_pair(TPin->at(i), V));
                T->at(1)[i].state = HPass;
            }
    }
    void resetMaze()
    {
        for (auto &i : *T)
            for (auto &ij : i)
            {
                ij.dist = INT_MAX;
                ij.visited = false;
            }
    }
    void addTrack()
    {
        track t;
        for (int x = 0; x < length; x++)
            t.push_back(T->at(*width - 2)[x].copyGrid(&T->at(*width - 1)[x]));
        T->insert(T->end() - 1, t);
        (*width)++;

        dbg.printAddTrack(); // for debug
        dbg.printTracks();   // for debug
    }
    inline bool isTarget(int x, int y, point &target) { return x == target.X && y == target.Y; }
    point extractMinDistPoint(vector<point> &v)
    {
        point p(v.front());
        int minDist = (*T)[p.Y][p.X].dist, index = 0, size = v.size(), i = 0;
        for (auto &itp : v)
        {
            if ((*T)[itp.Y][itp.X].dist < (*T)[p.Y][p.X].dist)
            {
                p = v[i];
                index = i;
                minDist = (*T)[itp.Y][itp.X].dist;
            }
            i++;
        }
        swap(v[index], v[size - 1]);
        v.pop_back();
        return p;
    }
    void BFS(point &start, point &target, int netID)
    {
        // initial
        vector<point> tmp;
        tmp.push_back(start);
        (*T)[start.Y][start.X].dist = 0;
        if ((*T)[start.Y - 1][start.X].dist == INT_MAX)
        {
            if ((*T)[start.Y - 1][start.X].existNet(netID))
            {
                (*T)[start.Y - 1][start.X].dist = (*T)[start.Y][start.X].dist;
                tmp.push_back(point(start.X, start.Y - 1));
                dbg.printDistOnTerminal(start.X, start.Y - 1); // for debug
            }
            else if ((*T)[start.Y - 1][start.X].canPass(D))
            {
                (*T)[start.Y - 1][start.X].dist = (*T)[start.Y][start.X].dist + 1;
                tmp.push_back(point(start.X, start.Y - 1));
                dbg.printDistOnTerminal(start.X, start.Y - 1); // for debug
            }
        }
        if ((*T)[start.Y + 1][start.X].dist == INT_MAX)
        {
            if ((*T)[start.Y + 1][start.X].existNet(netID))
            {
                (*T)[start.Y + 1][start.X].dist = (*T)[start.Y][start.X].dist;
                tmp.push_back(point(start.X, start.Y + 1));
                dbg.printDistOnTerminal(start.X, start.Y + 1); // for debug
            }
            else if ((*T)[start.Y + 1][start.X].canPass(U))
            {
                (*T)[start.Y + 1][start.X].dist = (*T)[start.Y][start.X].dist + 1;
                tmp.push_back(point(start.X, start.Y + 1));
                dbg.printDistOnTerminal(start.X, start.Y + 1); // for debug
            }
        }

        // BFS
        while (!tmp.empty())
        {
            point p = extractMinDistPoint(tmp);
            dbg.printDist(); // for debug

            if (p.Y > 1 && p.Y < *width - 2)
            {
                if ((*T)[p.Y - 1][p.X].dist == INT_MAX)
                {
                    if ((*T)[p.Y - 1][p.X].existNet(netID))
                    {
                        (*T)[p.Y - 1][p.X].dist = (*T)[p.Y][p.X].dist;
                        // tmp.push(point(p.X, p.Y - 1));
                        tmp.push_back(point(p.X, p.Y - 1));
                        dbg.printDistOnTerminal(p.X, p.Y - 1); // for debug
                    }
                    else if ((*T)[p.Y - 1][p.X].canPass(D))
                    {
                        (*T)[p.Y - 1][p.X].dist = (*T)[p.Y][p.X].dist + 1;
                        // tmp.push(point(p.X, p.Y - 1));
                        tmp.push_back(point(p.X, p.Y - 1));
                        dbg.printDistOnTerminal(p.X, p.Y - 1); // for debug
                    }
                }
                if ((*T)[p.Y + 1][p.X].dist == INT_MAX)
                {
                    if ((*T)[p.Y + 1][p.X].existNet(netID))
                    {
                        (*T)[p.Y + 1][p.X].dist = (*T)[p.Y][p.X].dist;
                        // tmp.push(point(p.X, p.Y + 1));
                        tmp.push_back(point(p.X, p.Y + 1));
                        dbg.printDistOnTerminal(p.X, p.Y + 1); // for debug
                    }
                    else if ((*T)[p.Y + 1][p.X].canPass(U))
                    {
                        (*T)[p.Y + 1][p.X].dist = (*T)[p.Y][p.X].dist + 1;
                        // tmp.push(point(p.X, p.Y + 1));
                        tmp.push_back(point(p.X, p.Y + 1));
                        dbg.printDistOnTerminal(p.X, p.Y + 1); // for debug
                    }
                }
                if (isTarget(p.X, p.Y + 1, target) || isTarget(p.X, p.Y - 1, target))
                    return;
            }

            if (p.X > 0 && p.X < length - 1)
            {
                if ((*T)[p.Y][p.X - 1].dist == INT_MAX)
                {
                    if ((*T)[p.Y][p.X - 1].existNet(netID))
                    {
                        (*T)[p.Y][p.X - 1].dist = (*T)[p.Y][p.X].dist;
                        // tmp.push(point(p.X - 1, p.Y));
                        tmp.push_back(point(p.X - 1, p.Y));
                        dbg.printDistOnTerminal(p.X - 1, p.Y); // for debug
                    }
                    else if ((*T)[p.Y][p.X - 1].canPass(L))
                    {
                        (*T)[p.Y][p.X - 1].dist = (*T)[p.Y][p.X].dist + 1;
                        // tmp.push(point(p.X - 1, p.Y));
                        tmp.push_back(point(p.X - 1, p.Y));
                        dbg.printDistOnTerminal(p.X - 1, p.Y); // for debug
                    }
                }
                if ((*T)[p.Y][p.X + 1].dist == INT_MAX)
                {
                    if ((*T)[p.Y][p.X + 1].existNet(netID))
                    {
                        (*T)[p.Y][p.X + 1].dist = (*T)[p.Y][p.X].dist;
                        tmp.push_back(point(p.X + 1, p.Y));
                        dbg.printDistOnTerminal(p.X + 1, p.Y); // for debug
                    }
                    else if ((*T)[p.Y][p.X + 1].canPass(R))
                    {
                        (*T)[p.Y][p.X + 1].dist = (*T)[p.Y][p.X].dist + 1;
                        tmp.push_back(point(p.X + 1, p.Y));
                        dbg.printDistOnTerminal(p.X + 1, p.Y); // for debug
                    }
                }
                if (isTarget(p.X + 1, p.Y, target) || isTarget(p.X - 1, p.Y, target))
                    return;
            }
        }
        cout << "no path exist." << endl; // for debug
    }
    point turnBack(point p)
    {
        while (1) // turn back until there are closer grids around
        {
            if (!(*T)[p.Y + 1][p.X].visited && (*T)[p.Y + 1][p.X].dist <= (*T)[p.Y][p.X].dist ||
                !(*T)[p.Y - 1][p.X].visited && (*T)[p.Y - 1][p.X].dist <= (*T)[p.Y][p.X].dist ||
                p.X < length - 1 && !(*T)[p.Y][p.X + 1].visited && (*T)[p.Y][p.X + 1].dist <= (*T)[p.Y][p.X].dist ||
                p.X > 0 && !(*T)[p.Y][p.X - 1].visited && (*T)[p.Y][p.X - 1].dist <= (*T)[p.Y][p.X].dist)
                break;

            (*T)[p.Y][p.X].dist = INT_MAX;

            if ((*T)[p.Y + 1][p.X].visited && (*T)[p.Y + 1][p.X].dist != INT_MAX)
                p.Y++;
            else if ((*T)[p.Y - 1][p.X].visited && (*T)[p.Y - 1][p.X].dist != INT_MAX)
                p.Y--;
            else if (p.X < length - 1 && (*T)[p.Y][p.X + 1].visited && (*T)[p.Y][p.X + 1].dist != INT_MAX)
                p.X++;
            else if (p.X > 0 && (*T)[p.Y][p.X - 1].visited && (*T)[p.Y][p.X - 1].dist != INT_MAX)
                p.X--;

            dbg.printDistOnTerminal(p.X, p.Y);
        }
        return p;
    }
    point findNearest(point &p, int netID)
    {
        int minDist = (*T)[p.Y][p.X].dist;
        point minP(p);

        if (!(*T)[p.Y + 1][p.X].visited && (*T)[p.Y + 1][p.X].dist <= minDist && (*T)[p.Y + 1][p.X].netState(netID) != Pin)
        {
            minDist = (*T)[p.Y + 1][p.X].dist;
            minP = make_pair(p.X, p.Y + 1);
        }
        if (!(*T)[p.Y - 1][p.X].visited && (*T)[p.Y - 1][p.X].dist <= minDist && (*T)[p.Y - 1][p.X].netState(netID) != Pin)
        {
            minDist = (*T)[p.Y - 1][p.X].dist;
            minP = make_pair(p.X, p.Y - 1);
        }
        if (p.X > 0 && !(*T)[p.Y][p.X - 1].visited && (*T)[p.Y][p.X - 1].dist <= minDist)
        {
            minDist = (*T)[p.Y][p.X - 1].dist;
            minP = make_pair(p.X - 1, p.Y);
        }
        if (p.X < length - 1 && !(*T)[p.Y][p.X + 1].visited && (*T)[p.Y][p.X + 1].dist <= minDist)
        {
            minDist = (*T)[p.Y][p.X + 1].dist;
            minP = make_pair(p.X + 1, p.Y);
        }
        if (minP == p) // went the wrong way
        {
            p = turnBack(p);
            return findNearest(p, netID);
        }
        else
        {
            (*T)[minP.Y][minP.X].visited = true;
            return minP;
        }
    }
    void backTrace(point &start, point &target, int netID)
    {
        point p1, p2(target), p3 = findNearest(target, netID);
        if (target.Y == 1)
            p1 = make_pair(target.X, 0);
        else
            p1 = make_pair(target.X, *width - 1);

        if (p3.X != p1.X && p3.Y != p1.Y)
        {
            (*T)[p2.Y][p2.X].net_state.insert(make_pair(netID, Via));
            (*T)[p2.Y][p2.X].state = NoPass;
        }
        else if (p3.X == p1.X)
        {
            (*T)[p2.Y][p2.X].net_state.insert(make_pair(netID, V));
            if ((*T)[p2.Y][p2.X].net_state.size() == 2)
                (*T)[p2.Y][p2.X].state = NoPass;
            else
                (*T)[p2.Y][p2.X].state = HPass;
        }
        else if (p3.Y == p1.Y)
        {
            (*T)[p2.Y][p2.X].net_state.insert(make_pair(netID, H));
            if ((*T)[p2.Y][p2.X].net_state.size() == 2)
                (*T)[p2.Y][p2.X].state = NoPass;
            else
                (*T)[p2.Y][p2.X].state = VPass;
        }

        while (p3 != start)
        {
            p1 = p2;
            p2 = p3;
            p3 = findNearest(p3, netID);
            if (p3.X != p1.X && p3.Y != p1.Y)
            {
                (*T)[p2.Y][p2.X].net_state.insert(make_pair(netID, Via));
                (*T)[p2.Y][p2.X].state = NoPass;
            }
            else if (p3.X == p1.X)
            {
                (*T)[p2.Y][p2.X].net_state.insert(make_pair(netID, V));
                if ((*T)[p2.Y][p2.X].net_state.size() == 2)
                    (*T)[p2.Y][p2.X].state = NoPass;
                else
                    (*T)[p2.Y][p2.X].state = HPass;
            }
            else if (p3.Y == p1.Y)
            {
                (*T)[p2.Y][p2.X].net_state.insert(make_pair(netID, H));
                if ((*T)[p2.Y][p2.X].net_state.size() == 2)
                    (*T)[p2.Y][p2.X].state = NoPass;
                else
                    (*T)[p2.Y][p2.X].state = VPass;
            }

            dbg.printDistOnTerminal(p3.X, p3.Y); // for debug
        }
        if (p3.Y == p2.Y)
        {
            (*T)[p3.Y][p3.X].net_state.insert(make_pair(netID, Via));
            (*T)[p3.Y][p3.X].state = NoPass;
        }
        else // p3.X == p2.X
        {
            (*T)[p3.Y][p3.X].net_state.insert(make_pair(netID, V));
            if ((*T)[p3.Y][p3.X].net_state.size() == 2)
                (*T)[p3.Y][p3.X].state = NoPass;
            else
                (*T)[p3.Y][p3.X].state = HPass;
        }
    }
    void mazeRouting(segment *s, int &nSegment)
    {
        dbg.printSegment(s); // for debug
        dbg.printTracks();   // for debug

        for (auto &it : s->out)
            it->indegree--;
        resetMaze();

        point start, target;
        if (T->at(0)[s->LX].existNet(s->netID))
        {
            start.X = s->LX;
            start.Y = 1;
        }
        else if (T->at(*width - 1)[s->LX].existNet(s->netID))
        {
            start.X = s->LX;
            start.Y = *width - 2;
        }
        if (T->at(0)[s->RX].existNet(s->netID))
        {
            target.X = s->RX;
            target.Y = 1;
        }
        else if (T->at(*width - 1)[s->RX].existNet(s->netID))
        {
            target.X = s->RX;
            target.Y = *width - 2;
        }

        BFS(start, target, s->netID);
        dbg.printDist(); // for debug
        dbg.printNets(); // for debug
        backTrace(start, target, s->netID);
        dbg.printDist(); // for debug
        dbg.printNets(); // for debug
        nSegment--;
    }
    bool tryFillTrack(deque<segment *> &S, int &nSegment)
    {
        auto *Tptr = &T->at(*width - 2);
        int LX;
        for (LX = 0; LX < length; LX++)
            if (Tptr->at(LX).state == HPass || Tptr->at(LX).state == Pass)
                break;

        int preFillNet = 0;
        for (int i = 0; i < nSegment;)
        {
            if (S[i]->indegree == 0 &&
                (S[i]->LX >= LX || (S[i]->LX == LX - 1 && S[i]->netID == preFillNet)))
            {
                if (tryAddSegment(S[i]) == EXIT_FAILURE)
                    return EXIT_FAILURE;
                dbg.printSegment(S[i]); // for debug
                dbg.printNets();        // for debug
                for (auto &ito : S[i]->out)
                    ito->indegree--;
                LX = S[i]->RX + 1;
                preFillNet = S[i]->netID;
                swap(S[i], S.back());
                S.pop_back();
                nSegment--;

                sort(S.begin(), S.end(),
                     [](segment *s1, segment *s2)
                     {
                         if (s1->indegree == s2->indegree)
                             return s1->LX < s2->LX;
                         else
                             return s1->indegree < s2->indegree;
                     });
                i = 0;
            }
            else
                i++;
        }
        return EXIT_SUCCESS;
    }
    void removeTrivialTrack()
    {
        int i = 1, size = T->size();
        for (; i < size - 1;)
        {
            bool trivial = true;
            for (int j = 0; j < length; j++)
            {
                int sizeU = T->at(i)[j].net_state.size(), sizeD = T->at(i - 1)[j].net_state.size();
                if (sizeU == 1)
                {
                    if (sizeD == 1)
                    {
                        int netID = T->at(i - 1)[j].net_state.begin()->first;
                        if (T->at(i)[j].existNet(netID))
                            continue;
                        else
                        {
                            trivial = false;
                            break;
                        }
                    }
                    else if (sizeD == 2)
                    {
                        int netID, direction;
                        tie(netID, direction) = *T->at(i)[j].net_state.begin();
                        if (T->at(i - 1)[j].existNet(netID) && T->at(i - 1)[j].netState(netID) == direction)
                            continue;
                        else
                        {
                            trivial = false;
                            break;
                        }
                    }
                }
                else if (sizeU == 2)
                {
                    if (sizeD == 1)
                    {
                        int netID, direction;
                        tie(netID, direction) = *T->at(i - 1)[j].net_state.begin();
                        if (T->at(i)[j].existNet(netID) && T->at(i)[j].netState(netID) == direction)
                            continue;
                        else
                        {
                            trivial = false;
                            break;
                        }
                    }
                    else if (sizeD == 2)
                    {
                        int netID1 = T->at(i)[j].net_state.begin()->first, netID2 = T->at(i)[j].net_state.rbegin()->first;
                        if (!T->at(i - 1)[j].existNet(netID1) || !T->at(i - 1)[j].existNet(netID2))
                        {
                            trivial = false;
                            break;
                        }
                    }
                }
            }

            if (trivial)
            {
                for (int j = 0; j < length; j++) // merge
                {
                    int sizeU = T->at(i)[j].net_state.size(), sizeD = T->at(i - 1)[j].net_state.size();
                    if (sizeU == 1)
                    {
                        if (sizeD == 0)
                            T->at(i - 1)[j] = T->at(i)[j];
                        else if (sizeD == 1)
                        {
                            int netIDU, netIDD, dirU, dirD;
                            tie(netIDU, dirU) = *T->at(i)[j].net_state.begin();
                            tie(netIDD, dirD) = *T->at(i - 1)[j].net_state.begin();
                            if (dirU == V && dirD == H || dirU == H && dirD == V) // H + V
                            {
                                T->at(i - 1)[j].net_state.insert(make_pair(netIDU, dirU));
                                T->at(i - 1)[j].state = NoPass;
                            }
                            else if (dirU == V && dirD == H || dirU == H && dirD == V) // H + Via
                            {
                                T->at(i - 1)[j].net_state.begin()->second = Via;
                                T->at(i - 1)[j].state = NoPass;
                            }
                        }
                    }
                    else if (sizeU == 2)
                    {
                        if (sizeD == 1)
                            T->at(i - 1)[j] = T->at(i)[j];
                    }
                }
                T->erase(T->begin() + i); // delete
                (*width)--;
                size--;
            }
            else
                i++;
        }
    }
};
struct channel
{
    int length; // lenth = # of pins
    int width;  // first track and last track are pins, # of routing_track = width - 2
    int nNets;
    map<int, int> nets; // netID & # of pins
    vector<int> TPin, BPin;
    vector<track> T;
    router rt;
    graph VCG;

    channel(vector<int> &_TPin, vector<int> &_BPin)
    {
        TPin = _TPin;
        BPin = _BPin;
        length = _TPin.size();
        width = 3;
        rt.initial(length, &width, &TPin, &BPin, &T, &VCG);
        VCG.initial(length, &TPin, &BPin, &nets);
        dbg.initial(&T, &width, length);

        track top, routingTrack, bot;

        // set pins
        for (const auto &it : TPin)
        {
            grid g;
            g.state = NoPass;
            g.net_state.insert(make_pair(it, Pin));
            top.push_back(g);

            if (it != 0)
            {
                auto itn = nets.find(it);
                if (itn != nets.end())
                    itn->second++;
                else
                    nets.insert(make_pair(it, 1));
            }
        }
        for (const auto &it : BPin)
        {
            grid g;
            g.state = NoPass;
            g.net_state.insert(make_pair(it, Pin));
            bot.push_back(g);

            if (it != 0)
            {
                auto itn = nets.find(it);
                if (itn != nets.end())
                    itn->second++;
                else
                    nets.insert(make_pair(it, 1));
            }
        }
        nNets = nets.size();

        // set routing track
        for (int i = 0; i < length; i++)
        {
            grid g;
            g.state = Pass;
            routingTrack.push_back(g);
        }

        T.push_back(bot);
        T.push_back(routingTrack);
        T.push_back(top);
    }
    void routing()
    {
        rt.columnConnect();
        dbg.printTracks(); // for debug

        VCG.buildGraph();

        deque<segment *> S;
        for (auto &it : VCG.S)
            S.push_back(&it);
        int nSegment = VCG.nSegment;
        segment *cycleBegin, *cycleEnd;
        while (!S.empty())
        {
            sort(S.begin(), S.end(),
                 [](segment *s1, segment *s2)
                 {
                     if (s1->indegree == s2->indegree)
                         return s1->LX < s2->LX;
                     else
                         return s1->indegree < s2->indegree;
                 });

            if (S[0]->indegree == 0)
            {
                if (S[0] == cycleEnd)
                {
                    rt.mazeRouting(cycleEnd, nSegment);
                    S.pop_front();
                }
                else if (rt.tryFillTrack(S, nSegment) == EXIT_FAILURE)
                {
                    dbg.printNets();     // for debug
                    dbg.printNetState(); // for debug
                    rt.mazeRouting(S[0], nSegment);
                    S.pop_front();
                }

                if (nSegment != 0)
                    rt.addTrack();
            }
            else
            {
                tie(cycleBegin, cycleEnd) = VCG.findCycle(S);
                cycleBegin->indegree = 0;
                rt.addTrack();
                dbg.printTracks(); // for debug
            }
        }
        dbg.printNets();     // for debug
        dbg.printNetState(); // for debug
        rt.removeTrivialTrack();
    }
};

struct parser
{
    pair<vector<int>, vector<int>> input(string &inf)
    {
        vector<int> TPin, BPin;

        ifstream ifs(inf);
        if (!ifs)
        {
            cout << "Input() failed.";
            return make_pair(TPin, BPin);
        }

        string line, tmp;
        getline(ifs, line);
        while (!isdigit(line.back())) // remove space at the end of line
            line.pop_back();
        istringstream iss1(line);
        while (!iss1.eof())
        {
            iss1 >> tmp;
            TPin.push_back(stoi(tmp));
        }
        getline(ifs, line);
        while (!isdigit(line.back())) // remove space at the end of line
            line.pop_back();
        istringstream iss2(line);
        while (!iss2.eof())
        {
            iss2 >> tmp;
            BPin.push_back(stoi(tmp));
        }
        ifs.close();
        return make_pair(TPin, BPin);
    }

    pair<point, int> findStartGrid(channel &ch, int netID)
    {
        point start;
        start.X = 0;
        for (; ch.TPin[start.X] != netID && ch.BPin[start.X] != netID;)
            start.X++;
        if (ch.TPin[start.X] == netID)
        {
            start.Y = ch.width - 1;
            return make_pair(start, U);
        }
        else
        {
            start.Y = 0;
            return make_pair(start, D);
        }
    }
    vector<point> findNext(channel &ch, queue<pair<point, int>> &p_fromDir, int netID)
    {
        int fromDir = p_fromDir.front().second;
        point p1 = p_fromDir.front().first;
        vector<point> p2;

        // dist is used as a visited mark, if via has been visited then set via.dist == 0
        if (fromDir != U)
        {
            int y;
            for (y = p1.Y; y < ch.width - 1 && ch.T[y + 1][p1.X].existNet(netID);) // going up
                if (ch.T[++y][p1.X].netState(netID) == Via)
                    break;

            if (y != p1.Y && ch.T[y][p1.X].visited == false)
            {
                point p(p1.X, y);
                ch.T[p.Y][p.X].visited = true;
                p2.push_back(p);
                p_fromDir.push(make_pair(p, D));
            }
        }
        if (fromDir != D)
        {
            int y;
            for (y = p1.Y; y > 0 && ch.T[y - 1][p1.X].existNet(netID);) // going down
                if (ch.T[--y][p1.X].netState(netID) == Via)
                    break;
            if (y != p1.Y && ch.T[y][p1.X].visited == false)
            {
                point p(p1.X, y);
                ch.T[p.Y][p.X].visited = true;
                p2.push_back(p);
                p_fromDir.push(make_pair(p, U));
            }
        }
        if (fromDir != R)
        {
            int x;
            for (x = p1.X; x < ch.length - 1 && ch.T[p1.Y][x + 1].existNet(netID);) // going right
                if (ch.T[p1.Y][p1.X].netState(netID) == Pin && ch.T[p1.Y][p1.X + 1].netState(netID) == Pin ||
                    ch.T[p1.Y][++x].netState(netID) == Via)
                    break;
            if (x != p1.X && ch.T[p1.Y][x].visited == false)
            {
                point p(x, p1.Y);
                ch.T[p.Y][p.X].visited = true;
                p2.push_back(p);
                p_fromDir.push(make_pair(p, L));
            }
        }
        if (fromDir != L)
        {
            int x;
            for (x = p1.X; x > 0 && ch.T[p1.Y][x - 1].existNet(netID);) // going left
                if (ch.T[p1.Y][p1.X].netState(netID) == Pin && ch.T[p1.Y][p1.X - 1].netState(netID) == Pin ||
                    ch.T[p1.Y][--x].netState(netID) == Via)
                    break;
            if (x != p1.X && ch.T[p1.Y][x].visited == false)
            {
                point p(x, p1.Y);
                ch.T[p.Y][p.X].visited = true;
                p2.push_back(p);
                p_fromDir.push(make_pair(p, R));
            }
        }

        return p2;
    }
    void outputSegment(ofstream &ofs, point &p1, point &p2)
    {
        if (p1.X == p2.X)
        {
            if (p1.Y < p2.Y)
                ofs << ".V " << p1.X << " " << p1.Y << " " << p2.Y << endl;
            else // p2.Y < p1.Y
                ofs << ".V " << p2.X << " " << p2.Y << " " << p1.Y << endl;
        }
        else // p1.Y == p2.Y
        {
            if (p1.X < p2.X)
                ofs << ".H " << p1.X << " " << p1.Y << " " << p2.X << endl;
            else // p2.X < p1.X
                ofs << ".H " << p2.X << " " << p2.Y << " " << p1.X << endl;
        }
    }
    void output(string &of, channel &ch)
    {
        ofstream ofs(of);
        if (!ofs)
        {
            cout << "Output() failed." << endl;
            return;
        }

        ch.rt.resetMaze();

        for (const auto &it : ch.nets)
        {
            ofs << ".begin " << it.first << endl;
            queue<pair<point, int>> p_fromDir;

            // set net's start grid
            p_fromDir.push(findStartGrid(ch, it.first));

            // print segment
            for (int i = 1; i < it.second;) // n pins -> n-1 wire
            {
                point p1 = p_fromDir.front().first;
                vector<point> p2 = findNext(ch, p_fromDir, it.first);

                for (auto &itp : p2)
                {
                    if (ch.T[itp.Y][itp.X].netState(it.first) == Pin)
                        i++;
                    outputSegment(ofs, p1, itp);
                }

                p_fromDir.pop();
            }

            ofs << ".end " << endl;
        }
    }
};

int main(int argc, char **argv)
{
    string inf = argc == 1 ? "case1.txt" : argv[1];
    string of = argc == 1 ? "output.txt" : argv[2];

    parser ps;
    pair<vector<int>, vector<int>> TB = ps.input(inf);
    if (TB.first.empty() || TB.second.empty())
        return EXIT_FAILURE;

    channel ch(TB.first, TB.second);
    ch.routing();

    dbg.printNets();     // for debug
    dbg.printNetState(); // for debug

    ps.output(of, ch);

    return EXIT_SUCCESS;
}