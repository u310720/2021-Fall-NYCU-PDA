#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>
#include <ctime>
#include <random>
#include <iomanip>
#include <climits>
using namespace std;
//#define DEBUG

int RUNTIME_LIMIT = 250;

string _case = "ami49"; // imported file names
mt19937 gen(time(NULL));
bool legal_flag = false; // if newF is legal, then switch cost function to continue optimizing area and wire
double time_start;

double alpha = 0.5; // user-define parameter
enum operation
{
    Rotate,
    Swap2Block_SPx1,
    Swap2Block_SPx2
};
// end define

// define for input data
double outlineW, outlineH;
int numBlocks;    // # of blocks
int numTerminals; // # of terminals
int numNets;      // # of nets
// end define

// basic component
// begin
struct block
{
    string name;
    int x, y, w, h;
    int pos_p, pos_n; // position index of seqence pair, floorplan->sp_p[pos_p], floorplan->sp_p[pos_n]

    block(string _name, int _w, int _h, int sp_p) : name(_name), x(0), y(0), w(_w), h(_h), pos_p(sp_p), pos_n(sp_p) {}
    block(block *b) : name(b->name), x(b->x), y(b->y), w(b->w), h(b->h) {}
    void rotate() { swap(this->w, this->h); }
    inline int centerX() { return this->x + this->w / 2; }
    inline int centerY() { return this->y + this->h / 2; }
    inline int area() { return w * h; }
};
struct terminal
{
    string name;
    int x, y;

    terminal(string _name, int _x, int _y) : name(_name), x(_x), y(_y) {}
};
struct pos_backup
{
    // if newF be rejected, than restore pos_p & pos_n
    bool rotated;
    vector<block *> blk;
    vector<pair<int, int>> pos_pn;

    pos_backup() : rotated(false) {}
    void backup(block *b)
    {
        this->blk.push_back(b);
        this->pos_pn.push_back(make_pair(b->pos_p, b->pos_n));
    }
    void restore()
    {
        if (!rotated)
        {
            int i = 0;
            for (auto &it : this->blk)
            {
                it->pos_p = pos_pn[i].first;
                it->pos_n = pos_pn[i].second;
                i++;
            }
        }
        else
            blk.front()->rotate();
        delete this;
    }
};
// end

// network
// begin
map<string, block *> mapBlock;       // for quickly wire blocks in the net
map<string, terminal *> mapTerminal; // for quickly wire terminals int the net
struct net
{
    vector<block *> blocks;       // blocks connected in the net
    vector<terminal *> terminals; // terminals connected in the net
    int degree;                   // net degree

    net(int _degree) : degree(_degree) {}
    int HPWL()
    {
        int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;
        for (const auto &it : this->blocks)
        {
            int centerX = it->centerX(), centerY = it->centerY();
            minX = centerX < minX ? centerX : minX;
            minY = centerY < minY ? centerY : minY;
            maxX = centerX > maxX ? centerX : maxX;
            maxY = centerY > maxY ? centerY : maxY;
        }
        for (const auto &it : this->terminals)
        {
            minX = it->x < minX ? it->x : minX;
            minY = it->y < minY ? it->y : minY;
            maxX = it->x > maxX ? it->x : maxX;
            maxY = it->y > maxY ? it->y : maxY;
        }
        return (maxX - minX) + (maxY - minY);
    }
};
vector<net *> allNet; // store all imformation from NETS file
// end

// floorplan, implemented by sequence pair
// begin
struct floorplan
{
    vector<block *> sp_p; // +SP
    vector<block *> sp_n; // -SP
    int cost, w, h;

    floorplan(floorplan *F) : sp_p(F->sp_p), sp_n(F->sp_n), cost(F->cost), w(F->w), h(F->h) {} // copy
    floorplan() : cost(0), w(0), h(0)
    {
        sp_p.clear();
        sp_n.clear();
    }

    inline bool legal() { return this->w <= outlineW && this->h <= outlineH; } // check the legality of this floorplan
    inline int area() { return w * h; }
    void placeBlocks()
    {
        // calculate the (1)coordinates of each block, (2)chip width and (3)chip height based on Sequence pair
        vector<int> L(numBlocks, 0);
        this->w = 0;
        this->h = 0;

        // calculate block.x
        for (auto itPos = this->sp_p.begin(); itPos != this->sp_p.end(); itPos++)
        {
            int p = 0;
            for (auto itNeg = this->sp_n.begin(); itNeg != this->sp_n.end(); itNeg++, p++)
                if (*itNeg == *itPos)
                    break;
            (*itPos)->x = L[p];
            int t = (*itPos)->x + (*itPos)->w;
            if (t > this->w)
                this->w = t;
            for (int j = p; j < numBlocks; j++)
            {
                if (t > L[j])
                    L[j] = t;
                else
                    break;
            }
        }
        // calculate block.y
        L.assign(numBlocks, 0);
        for (auto itPos = this->sp_p.rbegin(); itPos != this->sp_p.rend(); itPos++)
        {
            int p = 0;
            for (auto itNeg = this->sp_n.begin(); itNeg != this->sp_n.end(); itNeg++, p++)
                if (*itNeg == *itPos)
                    break;
            (*itPos)->y = L[p];
            int t = (*itPos)->y + (*itPos)->h;
            if (t > this->h)
                this->h = t;
            for (int j = p; j < numBlocks; j++)
            {
                if (t > L[j])
                    L[j] = t;
                else
                    break;
            }
        }
    }
    int Cost()
    {
        if (legal_flag)
        {
            int W = 0; // total HPWL
            for (const auto &it : allNet)
                W += it->HPWL();
            int A = this->area(); // bounding-box area of the floorplan, i.e., chip area
            return alpha * A + (1 - alpha) * W;
        }
        else
        {
            int outsideW = 0;
            if (this->w > outlineW)
                outsideW += this->w - outlineW;
            if (this->h > outlineH)
                outsideW += this->h - outlineH;
            return outsideW;
        }
    }
    void rotate(int i)
    {
        this->sp_p[i]->rotate();
    }
    void swap1(int i, int j, bool PorN)
    {
        if (PorN)
        {
            swap(this->sp_p[i], this->sp_p[j]);               // swap block ptr
            swap(this->sp_p[i]->pos_p, this->sp_p[j]->pos_p); // swap position index
        }
        else
        {
            swap(this->sp_n[i], this->sp_n[j]);               // swap block ptr
            swap(this->sp_n[i]->pos_n, this->sp_n[j]->pos_n); // swap position index
        }
    }
    void swap2(int i, int j)
    {
        swap(this->sp_p[i], this->sp_p[j]);                                       // swap block ptr
        swap(this->sp_n[this->sp_p[i]->pos_n], this->sp_n[this->sp_p[j]->pos_n]); // swap block ptr
        swap(this->sp_p[i]->pos_p, this->sp_p[j]->pos_p);                         // swap position index
        swap(this->sp_p[i]->pos_n, this->sp_p[j]->pos_n);                         // swap position index
    }
};
struct block_backup
{
    block *block_ptr;
    int x, y, w, h;

    block_backup(block *b) : block_ptr(b), x(b->x), y(b->y), w(b->w), h(b->h) {}
};
struct solution_backup
{
    int cost, W, area, w, h;
    vector<block_backup> blk;

    solution_backup() : cost(INT_MAX) {}
};
// end

#ifdef DEBUG
void printSP(floorplan *F)
{
    for (const auto &it : F->sp_p)
        cout << it->name << " ";
    cout << endl;
    for (const auto &it : F->sp_n)
        cout << it->name << " ";
    cout << endl;
}
void faultDetecter(floorplan *F)
{
    int i = 0, j = 0;
    for (const auto &it : F->sp_p)
        if (i++ != it->pos_p)
            system("pause");
        else
            cout << it->name << it->pos_p << " ";
    cout << endl;
    for (const auto &it : F->sp_n)
        if (j++ != it->pos_n)
            system("pause");
        else
            cout << it->name << it->pos_n << " ";
    cout << endl
         << endl;
}
void outputDEBUG(floorplan *F)
{
    ofstream ofs;
    ofs.open("DEBUG.txt");
    if (!ofs)
    {
        cerr << "DEBUG.txt could not open." << endl;
        return;
    }
    ofs << "%%writefile Graph.txt" << endl;
    int i = 1;
    for (const auto &it : F->sp_p)
        ofs << i++ << " " << it->x << " " << it->y << " " << it->w << " " << it->h << endl;
    ofs.close();
}
#endif

void rndPerturb(floorplan *F)
{
    int op = rand() % 3;
    int i, j;
    i = rand() % numBlocks; // randomly select a block in F->sp_p
    for (j = i; j == i;)    // randomly select another block in F->sp_p
        j = rand() % numBlocks;
    if (op == 0)
    {
        F->sp_p[i]->rotate();
    }
    else if (op == 1)
    {
        F->swap1(i, j, rand() % 2);
    }
    else if (op == 2)
    {
        F->swap2(i, j);
    }
    F->placeBlocks();
    F->cost = F->Cost();
}
pos_backup *perturb(floorplan *F, mt19937 &gen)
{
    uniform_real_distribution<float> rnd_operation(0.0, 3.0);
    uniform_real_distribution<float> rnd_block(0.0, numBlocks);

    pos_backup *pb = new pos_backup;
    switch ((int)rnd_operation(gen))
    {
    case Rotate:
    {
        // implemented by swap block's width and height
        int i = (int)rnd_block(gen);
        pb->rotated = true;
        pb->backup(F->sp_p[i]);
        F->sp_p[i]->rotate();
        break;
    }
    case Swap2Block_SPx1:
    {
        uniform_real_distribution<float> rnd_SP(0.0, 2.0);
        int i, j;
        i = rnd_block(gen);  // randomly select a block in F->sp_p
        for (j = i; j == i;) // randomly select another block in F->sp_p
            j = rnd_block(gen);
        pb->rotated = false;
        // F->swap1(i, j, (int)rnd_SP(gen));
        if ((int)rnd_SP(gen))
        {
            pb->backup(F->sp_p[i]);
            pb->backup(F->sp_p[j]);

            swap(F->sp_p[i], F->sp_p[j]);               // swap block ptr
            swap(F->sp_p[i]->pos_p, F->sp_p[j]->pos_p); // swap position index
        }
        else
        {
            pb->backup(F->sp_n[i]);
            pb->backup(F->sp_n[j]);
            swap(F->sp_n[i], F->sp_n[j]);               // swap block ptr
            swap(F->sp_n[i]->pos_n, F->sp_n[j]->pos_n); // swap position index
        }
        break;
    }
    case Swap2Block_SPx2:
    {
        int i, j;
        i = rnd_block(gen);  // randomly select a block in F->sp_p
        for (j = i; j == i;) // randomly select another block in F->sp_p
            j = rnd_block(gen);
        pb->rotated = false;
        pb->backup(F->sp_p[i]);
        pb->backup(F->sp_p[j]);
        // F->swap2(i, j);
        swap(F->sp_p[i], F->sp_p[j]);                                 // swap block ptr
        swap(F->sp_n[F->sp_p[i]->pos_n], F->sp_n[F->sp_p[j]->pos_n]); // swap block ptr
        swap(F->sp_p[i]->pos_p, F->sp_p[j]->pos_p);                   // swap position index
        swap(F->sp_p[i]->pos_n, F->sp_p[j]->pos_n);                   // swap position index
        break;
    }
    default:
    {
        // implemented by swap block's width and height
        int i = (int)rnd_block(gen);
        pb->rotated = true;
        pb->backup(F->sp_p[i]);
        F->sp_p[i]->rotate();
        break;
    }
    }

    F->placeBlocks();
    F->cost = F->Cost();
    return pb;
}
floorplan *greedyPerturb(floorplan *F, mt19937 &gen)
{
    floorplan *newF = new floorplan(F);
    pos_backup *pb = perturb(newF, gen);
    int delta = newF->cost - F->cost;
    bool newF_legal = newF->legal();
    if (newF_legal && legal_flag == false)
    {
        legal_flag = true;
        newF->cost = newF->Cost(); // get new cost after switch cost function
        delete F;
        return newF;
    }
    if (legal_flag)
    {
        if (newF_legal && delta <= 0)
        {
            delete F;
            return newF;
        }
        else
        {
            delete newF;
            pb->restore();
            return F;
        }
    }
    else
    {
        if (delta <= 0)
        {
            delete F;
            return newF;
        }
        else
        {
            delete newF;
            pb->restore();
            return F;
        }
    }
}
floorplan *SA(floorplan *F)
{
    int count = 0, n3 = pow(numBlocks, 3);

    legal_flag = false;
    for (int i = 0; i < 5000; i++)
        rndPerturb(F);
    do
    {
        F = greedyPerturb(F, gen);
    } while (!F->legal() && count < n3);

    if (F->legal())
        return F;
    else
        return SA(F);
}
floorplan *optimize(floorplan *F)
{
    int count = 0, oldcost, round_limit = 15000;
    do
    {
        oldcost = F->cost;
        F = greedyPerturb(F, gen);

        if (oldcost == F->cost)
            count++;
        else
            count = 0;

        //cout << F->cost << " " << count << endl; // DEBUG
    } while ((clock() - time_start) / CLOCKS_PER_SEC < RUNTIME_LIMIT && count < round_limit);

    return F;
}

void importBlockFile(ifstream &ifs, floorplan *F)
{
    string line, ss;

    ifs >> ss; // "Outline:"
    ifs >> ss; // outline width
    outlineW = stod(ss);
    ifs >> ss; // outline height
    outlineH = stod(ss);

    ifs >> ss; // "NumBlocks:"
    ifs >> ss; // # of Blocks
    numBlocks = stoi(ss);

    ifs >> ss; // "NumTerminals:"
    ifs >> ss; // # of terminals
    numTerminals = stoi(ss);

    int block_count = 0;
    while (!ifs.eof())
    {
        // import Blocks & terminals imformation
        getline(ifs, line);
        if (line.empty())
            continue;
        string name;
        istringstream iss(line);
        iss >> name;
        iss >> ss;
        if (ss == "terminal")
        {
            int x, y;
            iss >> ss;
            x = stoi(ss);
            iss >> ss;
            y = stoi(ss);
            terminal *t = new terminal(name, x, y);
            mapTerminal.insert(make_pair(name, t));
        }
        else if (!name.empty())
        {
            int w, h;
            w = stoi(ss);
            iss >> ss;
            h = stoi(ss);
            block *b = new block(name, w, h, block_count++);
            // block *b = new block(to_string(block_count), w, h, block_count++); // DEBUG, rename module as number
            F->sp_p.push_back(b);
            F->sp_n.push_back(b);
            mapBlock.insert(make_pair(name, b));
        }
    }
}
void importNetFile(ifstream &ifs)
{
    string line, ss;

    ifs >> ss; // "NumNets:"
    ifs >> ss; // # of Nets
    numNets = stoi(ss);
    while (!ifs.eof())
    {
        getline(ifs, line);
        if (line.empty())
            continue;
        istringstream iss(line);
        iss >> ss;
        if (ss == "NetDegree:")
        {
            iss >> ss; // degree
            net *n = new net(stoi(ss));
            for (int i = 0; i < n->degree; i++)
            {
                ifs >> ss; // terminal's or block's name
                auto itB = mapBlock.find(ss);
                if (itB != mapBlock.end())
                    n->blocks.push_back(itB->second);
                else
                {
                    auto itT = mapTerminal.find(ss);
                    if (itT != mapTerminal.end())
                        n->terminals.push_back(itT->second);
                }
            }
            allNet.push_back(n);
        }
    }
    mapBlock.clear();
    mapTerminal.clear();
}
floorplan *importData(string &BlockFile, string &NetFile)
{
    ifstream ifb, ifn;
    floorplan *F = new floorplan;
    ifb.open(BlockFile);
    ifn.open(NetFile);
    if (!ifb || !ifn)
        return nullptr;

    // import block imformation to F and set mapBlock, mapTerminal
    importBlockFile(ifb, F);
    ifb.close();

    // import net imformation to allNet
    importNetFile(ifn);
    ifn.close();

    return F;
}
solution_backup *backupSolution(floorplan *F)
{
    solution_backup *sb = new solution_backup;
    sb->cost = F->cost;
    sb->area = F->area();
    sb->w = F->w;
    sb->h = F->h;
    sb->W = 0; // total HPWL
    for (const auto &it : allNet)
        sb->W += it->HPWL();
    for (const auto &it : F->sp_p)
        sb->blk.push_back(block_backup(it));
    return sb;
}
void outputFloorplan(solution_backup *sb, string &OutFile, double time_start)
{
    ofstream ofs;
    ofs.open(OutFile);
    if (!ofs)
    {
        cerr << OutFile << " could not open." << endl;
        return;
    }
    ofs << sb->cost << endl;
    ofs << sb->W << endl;
    ofs << sb->area << endl;
    ofs << sb->w << " " << sb->h << endl;
    ofs << (clock() - time_start) / CLOCKS_PER_SEC << endl;
    for (const auto &it : sb->blk)
        ofs << it.block_ptr->name << " " << it.x << " " << it.y << " " << it.x + it.w << " " << it.y + it.h << endl;
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    time_start = clock();
    alpha = argc == 5 ? stod(argv[1]) : alpha;
    string BlockFile = argc == 5 ? argv[2] : _case + ".block";
    string NetFile = argc == 5 ? argv[3] : _case + ".nets";
    string OutFile = argc == 5 ? argv[4] : "output.rpt";

    // initial begin
    floorplan *F = importData(BlockFile, NetFile);
    if (F == nullptr)
    {
        cerr << "Could not import data." << endl;
        return EXIT_FAILURE;
    }
    F->placeBlocks();
    F->cost = F->Cost();
    // initial end

    solution_backup *sb = new solution_backup;
    do
    {
        F = SA(F);               // find fixed-outline floorplan
        F = optimize(F);         // optimize cost
        
        F->placeBlocks();        // Prevent bugs, have no time to deal with
        
        if (F->cost < sb->cost && F->legal())
        {
            delete sb;
            sb = backupSolution(F);
        }
    } while ((clock() - time_start) / CLOCKS_PER_SEC < RUNTIME_LIMIT);

    //outputDEBUG(F); // DEBUG
    outputFloorplan(sb, OutFile, time_start);

    return 0;
}
