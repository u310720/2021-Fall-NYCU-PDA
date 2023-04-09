// Best version
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <stdexcept> // for invalid_argument
#include <climits>
// #include <ctime>
using namespace std;

// basic math function
inline int abs(int x) { return x < 0 ? -x : x; }

// basic components
enum nodeType
{
    Cell,
    Terminal,
    Terminal_NI
};
struct node
{
    string name;
    int x_place, y_place; // position in gloabal placement
    int x_legal, y_legal; // position int legal placement
    int w, h;             // width, height
    int weight;
    int type;

    node(string _name, int _w, int _h, int _type)
        : name(_name), w(_w), h(_h), weight(-1), type(_type),
          x_place(-1), y_place(-1), x_legal(-1), y_legal(-1) {}
};
struct cluster
{
    vector<node *> cell; // all cells in this cluster, get nFirst by calling cell.front(), get nLast by calling cell.back()
    int x, w;            // optimal position of cluster and cluster's width
    int ec;              // weight, here is number of cells in this cluster
    int qc;              // used to give the optimal position, x = qc/ec

    cluster(node *ni) : x(ni->x_place), w(ni->w), ec(1), qc(ni->x_place) { cell.push_back(ni); }
    void addCell(node *ni)
    {
        ec++;
        qc += ni->x_place - w;
        x = qc / ec;
        w += ni->w;
        cell.push_back(ni);
    }
    void addCluster(cluster *ci) // merge the cluster ci, ci is on the right of this cluster
    {
        for (const auto &ni : ci->cell)
            cell.push_back(ni);
        ec += ci->ec;
        qc += ci->qc - ci->ec * w;
        x = qc / ec;
        w += ci->w;
    }
};
struct row
{
    int x, y;     // coordinates of the bottom-left corner of this row, x is subrowOrigin
    int w, h;     // width, height
    int nC;       // number of cluster int this row
    int resSpace; // available space int this row
    vector<cluster *> C;

    row(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h), nC(0), resSpace(_w) {}
    row(int _x, int _y, int _h, int _wSite, int _nSite)
        : x(_x), y(_y), w(_wSite * _nSite), h(_h), nC(0), resSpace(_wSite * _nSite) {}
    void placeCell() // place all cells in this row
    {
        for (auto &itc : C)
        {
            int nx = itc->x;
            for (auto &itn : itc->cell)
            {
                itn->y_legal = y;
                itn->x_legal = nx;
                nx += itn->w;
            }
        }
    }
    void placeCluster() // place all cluster and merge overlapping clusters
    {
        int rx_end = x + w;
        for (int i = nC - 1; i > 0; i--)
        {
            C[i]->x = C[i]->qc / C[i]->ec;
            if (C[i]->x + C[i]->w > rx_end)
                C[i]->x = rx_end - C[i]->w;
            if (C[i]->x < C[i - 1]->x + C[i - 1]->w) // overlap, merge clusters
            {
                C[i - 1]->addCluster(C[i]);
                delete C[i];
                C.pop_back();
                nC--;
            }
            else
                break;
        }
        if (C[0]->x < x)
            C[0]->x = x;
        else if (C[0]->x + C[0]->w > rx_end)
            C[0]->x = rx_end - C[0]->w;
    }
    void insert(node *ni)
    {
        cluster *newc = new cluster(ni);
        C.push_back(newc);
        resSpace -= ni->w;
        nC++;
        placeCluster();
    }
    int calcX(int xi, int wi) // when overlap, input ni->x_place and ni->w, return x_legal of the cell after collapse, only calculate and will not collapse the cluster
    {
        int cur_cx = xi, cur_cw = wi, cur_ec = 1, cur_qc = xi;
        int pre_cx = C.back()->x, pre_cw = C.back()->w, pre_ec = C.back()->ec, pre_qc = C.back()->qc;

        pre_ec += cur_ec;
        pre_qc += cur_qc - cur_ec * pre_cw;
        pre_cx = pre_qc / pre_ec;
        pre_cw += cur_cw;

        for (int i = nC - 1; i > 0; i--)
        {
            cur_cx = pre_cx;
            cur_cw = pre_cw;
            cur_ec = pre_ec;
            cur_qc = pre_qc;

            pre_cx = C[i - 1]->x;
            pre_cw = C[i - 1]->w;
            pre_ec = C[i - 1]->ec;
            pre_qc = C[i - 1]->qc;

            if (cur_cx < pre_cx + pre_cw) // overlap, merge clusters
            {
                pre_ec += cur_ec;
                pre_qc += cur_qc - cur_ec * pre_cw;
                pre_cx = pre_qc / pre_ec;
                pre_cw += cur_cw;
            }
            else
                break;
        }

        if (nC == 1)
            return pre_cx < x ? x + pre_cw : pre_cx + pre_cw;
        else
            return cur_cx < x ? x + cur_cw : cur_cx + cur_cw;
    }
    int trial(node *ni) // evaluate the cost of inserting node ni into this row
    {
        if (ni->w > resSpace) // this row can not accommodate cell ni
            return INT_MAX;

        int rx_end = x + w;
        if (C.empty())
        {
            if (ni->x_place < x)
                return abs(ni->x_place - x) + abs(ni->y_place - y);
            else if (ni->x_place + ni->w > rx_end)
                return abs(ni->x_place - (rx_end - ni->w)) + abs(ni->y_place - y);
            else
                return abs(ni->y_place - y);
        }
        else
        {
            if (ni->x_place + ni->w > rx_end) // on the right side of this row
                return abs(ni->x_place - (rx_end - ni->w)) + abs(ni->y_place - y);
            else // inside or to the left of this row
            {
                int cx_end = C[nC - 1]->x + C[nC - 1]->w;
                if (ni->x_place >= cx_end) // no overlap
                    return abs(ni->y_place - y);
                else // overlap
                    return abs(ni->x_place - calcX(ni->x_place, ni->w)) + abs(ni->y_place - y);
            }
        }
    }
};
struct splitTmp // for function splitRows()
{
    row *r;                 // row to be split
    vector<int> begin, end; // begin = terminal->x, end = terminal->x + terminal->w

    splitTmp(row *_r) : r(_r) {}
};
struct placement_data
{
    // "nABC" means number of ABC
    int nNode, nTerminal;       // according to input.nodes
    int x1, y1, x2, y2;         // placement's bottom-left corner and top-right corner
    vector<node *> nodes;       // all nodes
    vector<node *> cell;        // nodes to be legalized
    vector<node *> terminal;    // fixed nodes
    vector<node *> terminal_NI; // fixed nodes but overlap is allowed
    vector<row *> rows;         // all rows

    placement_data() : nNode(0), nTerminal(0), x1(INT_MAX), y1(INT_MAX), x2(INT_MIN), y2(INT_MIN) {}
    void ascendingSort(vector<node *> &v) // sort nodes in ascending order by x_place
    {
        sort(v.begin(), v.end(),
             [](node *n1, node *n2)
             { return n1->x_place < n2->x_place; });
    }
    void descendingSort(vector<node *> &v) // sort nodes in descending order by x_place
    {
        sort(v.begin(), v.end(),
             [](node *n1, node *n2)
             { return n1->x_place > n2->x_place; });
    }
    void split(splitTmp &tmp) // sub function of splitRows()
    {
        int rx_begin = tmp.r->x, rx_end = tmp.r->x + tmp.r->w;
        if (tmp.begin.front() == rx_begin && tmp.end.front() == rx_end) // completely coverd, delete this row
        {
            swap(tmp.r, rows.back());
            rows.pop_back();
            delete tmp.r;
        }
        else
        {
            int size = tmp.begin.size();
            for (int i = 0; i < size; i++)
            {
                rx_begin = tmp.r->x; // update rx_begin but rx_end no need to update

                if (tmp.begin[i] > rx_begin && tmp.end[i] == rx_end) // case: r'-coverd
                    tmp.r->w = tmp.begin[i] - rx_begin;
                else if (tmp.begin[i] == rx_begin && tmp.end[i] < rx_end) // case: coverd-r'
                {
                    tmp.r->x = tmp.end[i];
                    tmp.r->w = rx_end - tmp.end[i];
                }
                else // case: newR-coverd-r', i.e. (rx_begin < tmp.begin[i] && tmp.end[i] < rx_end)
                {
                    tmp.r->x = tmp.end[i];
                    tmp.r->w = rx_end - tmp.end[i];
                    row *newR = new row(rx_begin, tmp.r->y, tmp.begin[i] - rx_begin, tmp.r->h);
                    rows.push_back(newR);
                }
                tmp.r->resSpace = tmp.r->w;
            }
        }
    }
    void splitRows() // split rows according to the position occupied by the terminals
    {
        ascendingSort(terminal);
        vector<splitTmp> r_split; // rows to be split
        for (const auto &r : rows)
        {
            splitTmp tmp(r);
            int rx_begin = r->x, rx_end = r->x + r->w, ry_begin = r->y, ry_end = r->y + r->h; // row's endpoints
            for (const auto &t : terminal)
            {

                int ty_begin = t->y_place, ty_end = t->y_place + t->h;
                if (ty_end <= ry_begin || ty_begin >= ry_end) // terminal t does not cover row r
                    continue;

                int tx_begin = t->x_place, tx_end = t->x_place + t->w; // terminal's endpoints
                if (tx_begin < rx_begin)
                {
                    if (tx_end > rx_end)
                    {
                        tmp.begin.push_back(rx_begin);
                        tmp.end.push_back(rx_end);
                    }
                    else if (rx_begin < tx_end && tx_end <= rx_end)
                    {
                        tmp.begin.push_back(rx_begin);
                        tmp.end.push_back(tx_end);
                    }
                }
                else if (rx_begin <= tx_begin && tx_begin < rx_end)
                {
                    if (tx_end > rx_end)
                    {
                        tmp.begin.push_back(tx_begin);
                        tmp.end.push_back(rx_end);
                    }
                    else if (rx_begin < tx_end && tx_end <= rx_end)
                    {
                        tmp.begin.push_back(tx_begin);
                        tmp.end.push_back(tx_end);
                    }
                }
            }

            if (!tmp.begin.empty())
                r_split.push_back(tmp);
        }
        for (auto &r : r_split)
            split(r);
    }
    void sortRows() // sort rows by row->y
    {
        sort(rows.begin(), rows.end(),
             [](row *r1, row *r2)
             { return r1->y < r2->y; });
    }

    // functions for debug
    void checkNodesSort(bool mode) // mode: 0->descending 1->ascending
    {
        if (mode) // ascending
        {
            for (int i = 1; i < nNode; i++)
                if (nodes[i - 1]->x_place > nodes[i]->x_place)
                    cout << "ERROR!" << endl;
        }

        else // descending
        {
            for (int i = 1; i < nNode; i++)
                if (nodes[i - 1]->x_place < nodes[i]->x_place)
                    cout << "ERROR!" << endl;
        }
    }
    void outputRowsEndpoint()
    {
        sortRows();
        ofstream ofs("DEBUG.txt");
        int count = 1;
        for (const auto &r : rows)
            ofs << count++ << " " << r->x << " " << r->y << " " << r->w << " " << r->h << endl;
    }
};

// file I/O
struct parser
{
    placement_data *pData;

    // input
    void read_nodes(string &inf_nodes)
    {
        string line, name, w, h, tmp;
        node *n; // new node_ptr
        ifstream ifs(inf_nodes);
        if (!ifs)
            throw invalid_argument("read_nodes() failed.");

        // skip input.nodes header comments and set pData->nNode & pData->nTerminal
        while (pData->nNode == 0)
        {
            ifs >> tmp;
            if (tmp == "NumNodes")
            {
                ifs >> tmp >> tmp;
                pData->nNode = stoi(tmp);
                ifs >> tmp >> tmp >> tmp;
                pData->nTerminal = stoi(tmp);
            }
        }

        // read node's width, height and type
        ifs >> name >> w >> h >> tmp;
        for (int i = 0; i < pData->nNode; i++)
        {
            if (tmp.back() == 'l') // terminal
            {
                n = new node(name, stoi(w), stoi(h), Terminal);
                pData->terminal.push_back(n);
                pData->nodes.push_back(n);
                ifs >> name >> w >> h >> tmp;
            }
            else if (tmp.back() == 'I') // terminal_NI
            {
                n = new node(name, stoi(w), stoi(h), Terminal_NI);
                pData->terminal_NI.push_back(n);
                pData->nodes.push_back(n);
                ifs >> name >> w >> h >> tmp;
            }
            else // cell
            {
                n = new node(name, stoi(w), stoi(h), Cell);
                pData->cell.push_back(n);
                pData->nodes.push_back(n);
                name = tmp;
                ifs >> w >> h >> tmp;
            }
        }
        ifs.close();
    }
    void read_pl(string &inf_pl)
    {
        string line, x, y, tmp;
        ifstream ifs(inf_pl);
        if (!ifs)
            throw invalid_argument("read_pl() failed.");

        // skip input.pl header comments
        getline(ifs, line);
        getline(ifs, line);
        getline(ifs, line);

        // read nodes' x_place & y_place
        ifs >> tmp >> x >> y >> tmp >> tmp >> tmp;
        for (int i = 0; i < pData->nNode; i++)
        {
            pData->nodes[i]->x_place = stoi(x);
            pData->nodes[i]->y_place = stoi(y);
            if (isdigit(tmp.back()))
                ifs >> x >> y >> tmp >> tmp >> tmp;
            else
                ifs >> tmp >> x >> y >> tmp >> tmp >> tmp;
        }
        ifs.close();
    }
    void read_scl(string &inf_scl)
    {
        string tmp;
        ifstream ifs(inf_scl);
        row *r;
        if (!ifs)
            throw invalid_argument("read_scl() failed.");

        // skip input.scl header comments and get nRow
        int nRow = 0;
        while (nRow == 0)
        {
            ifs >> tmp;
            if (tmp == "NumRows")
            {
                ifs >> tmp >> tmp;
                nRow = stoi(tmp);
            }
        }

        // read row's information
        string x, y, h, wSite, nSite;
        for (int i = 0; i < nRow; i++)
        {
            ifs >> tmp >> tmp;          // CoreRow Horizontal
            ifs >> tmp >> tmp >> y;     // Coordinate    :   459
            ifs >> tmp >> tmp >> h;     // Height        :   12
            ifs >> tmp >> tmp >> wSite; // Sitewidth     :    1
            ifs >> tmp >> tmp >> tmp;   // Sitespacing   :    1
            ifs >> tmp >> tmp >> tmp;   // Siteorient    :    1
            ifs >> tmp >> tmp >> tmp;   // Sitesymmetry  :    1
            ifs >> tmp >> tmp >> x;     // SubrowOrigin  :    459
            ifs >> tmp >> tmp >> nSite; // NumSites      :   10692
            ifs >> tmp;                 // End

            r = new row(stoi(x), stoi(y), stoi(h), stoi(wSite), stoi(nSite));
            pData->rows.push_back(r);

            pData->x1 = r->x < pData->x1 ? r->x : pData->x1;
            pData->y1 = r->y < pData->y1 ? r->y : pData->y1;
            pData->x2 = r->x + r->w > pData->x2 ? r->x + r->w : pData->x2;
            pData->y2 = r->y + r->h > pData->y2 ? r->y + r->h : pData->y2;
        }
        ifs.close();
    }
    placement_data *read(string &inf_aux)
    {
        pData = new placement_data;

        string inf_pl, inf_scl, inf_nodes;
        ifstream ifs(inf_aux);
        if (!ifs)
        {
            cerr << "Could not open " << inf_aux << endl;
            throw invalid_argument("parser_main() failed.");
        }
        while (!ifs.eof())
        {
            string s;
            ifs >> s;
            if (s.find(".pl") != string::npos)
                inf_pl = s;
            else if (s.find(".scl") != string::npos)
                inf_scl = s;
            else if (s.find(".nodes") != string::npos)
                inf_nodes = s;
        }

        try
        {
            read_nodes(inf_nodes);
            read_pl(inf_pl);
            read_scl(inf_scl);
        }
        catch (invalid_argument &error_msg)
        {
            cerr << error_msg.what() << endl;
            throw invalid_argument("parser_main() failed.");
        }
        return pData;
    }

    // output
    void outputResult() // output legalization results
    {
        ofstream ofs("output.pl");
        ofs << "\n\n\n\n"; // output format, i.e. output blank header comment
        for (const auto &it : pData->nodes)
        {
            if (it->type == Cell)
                ofs << it->name << " " << it->x_legal << " " << it->y_legal << endl;
            else if (it->type == Terminal)
                ofs << it->name << " " << it->x_place << " " << it->y_place << " : N /FIXED" << endl;
            else if (it->type == Terminal_NI)
                ofs << it->name << " " << it->x_place << " " << it->y_place << " : N /FIXED_NI" << endl;
        }
    }
};

struct Abacus
{
    string inf_aux;

    Abacus(string &_inf_aux) : inf_aux(_inf_aux) {}
    void legalization(placement_data *pData) // main function of legalization
    {
        int partH, partW; // size of partitioning, try row which with y in the interval [cell->y - partSize, cell->y + partSize]
        if (inf_aux == "adaptec1.aux")
        {
            partH = 600;
            partW = 1000;
        }
        else if (inf_aux == "newblue5.aux")
        {
            partH = 1100;
            partW = 600;
        }
        else
        {
            partH = 1000;
            partW = 10000;
        }

        pData->ascendingSort(pData->cell); // Sort cells according to x-position;
        pData->sortRows();

        int y1Boundry = pData->y1 + 2 * partH;
        int y2Boundry = pData->y2 - 2 * partH;
        int x1Boundry = pData->x1 + 2 * partW;

        for (auto &itn : pData->cell)
        {
            int cost = INT_MAX, min_cost = INT_MAX; // cost & best cost
            row *rBest = pData->rows.front();       // best row

            if (itn->y_place < pData->y1)
            {
                int XplusW = itn->x_place + partW;

                for (auto &itr : pData->rows)
                {
                    if (itr->y > y1Boundry)
                        break;

                    if (itn->x_place < pData->x1 && itr->x > x1Boundry)
                        continue;
                    else if (itr->x > XplusW)
                        continue;

                    cost = itr->trial(itn);
                    if (cost < min_cost)
                    {
                        min_cost = cost;
                        rBest = itr;
                    }
                }
                rBest->insert(itn);
            }
            else if (itn->y_place > pData->y2)
            {
                int XplusW = itn->x_place + partW;

                for (auto itr = pData->rows.rbegin(); itr != pData->rows.rend(); itr++)
                {
                    if ((*itr)->y < y2Boundry)
                        break;

                    if (itn->x_place < pData->x1 && (*itr)->x > x1Boundry)
                        continue;
                    else if ((*itr)->x > XplusW)
                        continue;

                    cost = (*itr)->trial(itn);
                    if (cost < min_cost)
                    {
                        min_cost = cost;
                        rBest = *itr;
                    }
                }
                rBest->insert(itn);
            }
            else
            {
                int XplusW = itn->x_place + partW;

                for (auto &itr : pData->rows)
                {
                    if (itr->y < itn->y_place - partH)
                        continue;
                    else if (itr->y > itn->y_place + partH)
                        break;

                    if (itn->x_place < pData->x1 && itr->x > x1Boundry)
                        continue;
                    else if (itr->x > XplusW)
                        continue;

                    cost = itr->trial(itn);
                    if (cost < min_cost)
                    {
                        min_cost = cost;
                        rBest = itr;
                    }
                }
                rBest->insert(itn);
            }
        }
        for (auto &itr : pData->rows)
            itr->placeCell();
    }
};

int main(int argc, char **argv)
{
    // double t0 = clock();

    string inf_aux = argc == 1 ? "adaptec1.aux" : argv[1];
    parser fileIO;
    placement_data *pData;
    try
    {
        pData = fileIO.read(inf_aux);
    }
    catch (invalid_argument &error_msg)
    {
        cerr << error_msg.what() << endl;
        return EXIT_FAILURE;
    }

    pData->splitRows();

    Abacus abacus(inf_aux);
    abacus.legalization(pData);

    fileIO.outputResult();

    // cout << "Excution time = " << (clock() - t0) / CLOCKS_PER_SEC << "sec" << endl;
    return EXIT_SUCCESS;
}