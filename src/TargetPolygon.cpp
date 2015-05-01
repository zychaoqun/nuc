#include "TargetPolygon.h"
#include <algorithm>
#include <GL/glut.h>
#include "ros/ros.h"

#define ANGLE(a,b,c) (acos( ((a-b)*(c-b)) / (sqrt((a-b)*(a-b))*sqrt((c-b)*(c-b))) ))
#define MIN(a,b) ((a<b)?a:b)
#define MAX(a,b) ((a<b)?b:a)
#define D2(a,b) (a-b)*(a-b)

using namespace std;

TargetPolygon::TargetPolygon(vector<CNode *> &cs)
{
    std::copy(cs.begin(), cs.end(), std::back_inserter(cells));
    label = cells[0]->label;
    height = 0;
    base_idx[0] = -1;
    base_idx[1] = -1;

    Vector<4> fp = cs.back()->footPrint;
    cellW = fp[2]-fp[0];

    ConvexHull();
    FindBaseEdge();
    PlanLawnmower();
}

TargetPolygon::~TargetPolygon()
{
}

void TargetPolygon::GetLawnmowerPlan(vector<Vector<3> >  & v)
{
    std::copy(lm.begin(), lm.end(), std::back_inserter(v));
}

void TargetPolygon::MarkAsVisited()
{
    vector<CNode*>::iterator it = cells.begin();
    while(it != cells.end())
    {
        (*it++)->SetTreeVisited(true);
    }
}

void TargetPolygon::ConvexHull()
{
    // first node is the bottom most node
    unsigned first=0;
    for(unsigned int i=1; i<cells.size(); i++)
        if(cells[first]->pos[1]> cells[i]->pos[1])
            first = i;

    ch.push_back(cells[first]);
    //v.erase(v.begin()+first);

    if(cells.size()<=1)
        return;

    //second node
    unsigned second=1;
    Vector<3> p1 = ch[0]->pos - makeVector(-1, 0, 0);
    Vector<3> p2 = ch.back()->pos;

    double ang = ANGLE(p1, p2, cells[1]->pos);
    for(unsigned int i=2; i<cells.size(); i++)
    {
        double a = ANGLE(p1, p2, cells[i]->pos);
        if(a> ang)
        {
            ang = a;
            second = i;
        }
    }

    ch.push_back(cells[second]);

    if(cells.size() <=2)
        return;

    //int n=0;
    // the rest of the nodes
    while(true)
    {
        //if(n++ > 100) return;

        p1 = ch[ch.size()-2]->pos;
        p2 = ch.back()->pos;
        int next = -1;
        ang = -1;
        for(unsigned int i=0; i<cells.size(); i++)
        {
            if(cells[i] == ch.back())
                continue;

            double a = ANGLE(p1, p2, cells[i]->pos);
            if(a> ang)
            {
                ang = a;
                next = i;
            }
        }

        if(std::find(ch.begin(), ch.end(), cells[next]) != ch.end())
            break;
        else
            ch.push_back(cells[next]);
    }


    // remove extra nodes (colinear edges)
    int sz = ch.size();

    for(int i=0; i < ch.size(); i++)
    {
        sz = ch.size();

        int i1 = ((i-1)+sz)%sz;
        int i2 = i;
        int i3 = (i+1)%sz;

        Vector<3> v = ch[i2]->pos - ch[i1]->pos;
        Vector<3> u = ch[i3]->pos - ch[i2]->pos;

        Vector<3> r = u^v;

        if(sqrt(r*r) < 0.1)
        {
            ch.erase(ch.begin()+i);
            i--;
        }
    }

}

void TargetPolygon::FindBaseEdge()
{
    int sz = ch.size();
    double minHeight = 999999;
    int min_idx = -1;

    if(sz <= 2)
    {
        base_idx[0] = base_idx[1] = 0;
        return;
    }

    for(int i=0; i < sz; i++)
    {
        double maxHeight = 0;

        for(int j=0; j < sz; j++)
        {
            if(j==i || (i+1)%sz ==j)
                continue;

            double h = pointToLineDist(ch[i]->GetMAVWaypoint(), ch[(i+1)%sz]->GetMAVWaypoint(), ch[j]->GetMAVWaypoint());
            maxHeight = (maxHeight < h) ? h : maxHeight;
        }

        if(minHeight > maxHeight)
        {
            minHeight = maxHeight;
            height = minHeight;
            min_idx = i;
        }
    }

    double sd = pointToLineSignedDist(ch[min_idx]->GetMAVWaypoint(), ch[(min_idx+1)%sz]->GetMAVWaypoint(), ch[(min_idx+2)%sz]->GetMAVWaypoint());
    if(sd > 0)
    {
        base_idx[0] = min_idx;
        base_idx[1] = (min_idx+1)%sz;
        //return std::pair<int,int>(min_idx, (min_idx+1)%sz);
    }
    else
    {
        base_idx[1] = min_idx;
        base_idx[0] = (min_idx+1)%sz;
        //return std::pair<int,int>((min_idx+1)%sz, min_idx);
    }
}

double TargetPolygon::pointToLineDist(Vector<3> p1, Vector<3> p2, Vector<3> x)
{
    double d = fabs((p2[0]-p1[0])*(p1[1]-x[1]) - (p1[0]-x[0])*(p2[1]-p1[1]))/sqrt((p2[0]-p1[0])*(p2[0]-p1[0])+(p2[1]-p1[1])*(p2[1]-p1[1]));
    return d;
}

double TargetPolygon::pointToLineSignedDist(Vector<3> p1, Vector<3> p2, Vector<3> x)
{
    double d = ((p2[0]-p1[0])*(p1[1]-x[1]) - (p1[0]-x[0])*(p2[1]-p1[1]))/sqrt((p2[0]-p1[0])*(p2[0]-p1[0])+(p2[1]-p1[1])*(p2[1]-p1[1]));
    return d;
}

bool TargetPolygon::GetLineSegmentIntersection(Vector<3> p0, Vector<3> p1, Vector<3> p2, Vector<3> p3, Vector<3> &intersection_p)
{
    float p0_x = p0[0], p0_y = p0[1], p1_x = p1[0], p1_y=p1[1], p2_x = p2[0], p2_y=p2[1], p3_x = p3[0], p3_y=p3[1];

    float s1_x, s1_y, s2_x, s2_y;
    s1_x = p1_x - p0_x;     s1_y = p1_y - p0_y;
    s2_x = p3_x - p2_x;     s2_y = p3_y - p2_y;

    float s, t;
    s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y);
    t = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
    {
        // Collision detected
        //if (i_x != NULL)
        intersection_p[0] = p0_x + (t * s1_x);
        intersection_p[1] = p0_y + (t * s1_y);
        return true;
    }

    return false; // No collision

}

void TargetPolygon::PlanLawnmower()
{
    double interlap_d = 3*cellW;

    if(base_idx[0] == base_idx[1])
        return ;

    Vector<3> baseDir  = ch[base_idx[1]]->GetMAVWaypoint() - ch[base_idx[0]]->GetMAVWaypoint();
    Vector<3> baseDirNorm = baseDir;
    baseDirNorm[2] = 0;
    normalize(baseDirNorm);

    //clockwise orthogonal vector
    Vector<3> sweepDir = makeVector(baseDir[1], -baseDir[0], 0);
    normalize(sweepDir);

    Vector<3> sn0 = ch[base_idx[0]]->GetMAVWaypoint();
    Vector<3> en0 = ch[base_idx[1]]->GetMAVWaypoint();
    double offset0 = interlap_d*0.5;

    sn0 += (offset0-cellW/2) * sweepDir;
    en0 += (offset0-cellW/2) * sweepDir;

    // first lm track
    lm.push_back(sn0);
    lm.push_back(en0);

    double n =-1;
    while(true)
    {
        if(n > 30)
            break;

        n+=1.0;

        Vector<3> sn = sn0;
        Vector<3> en = en0;
        sn += n * interlap_d * sweepDir;
        en += n * interlap_d * sweepDir;

        sn -= 50.0 * baseDirNorm;
        en += 50.0 * baseDirNorm;

        vector<Vector<3> > intersections;

        for(int i=0; i< ch.size(); i++)
        {
            Vector<3> ise =makeVector(0,0, sn0[2]);
            if(GetLineSegmentIntersection(sn, en, ch[i]->GetMAVWaypoint(), ch[(i+1)%(ch.size())]->GetMAVWaypoint(), ise))
            {
                intersections.push_back(ise);
            }
        }

        if(intersections.size()<=1)
        {
           //ROS_INFO("No lawnmower track found ...");
           //break;
        }
        else
        {
            if(intersections.size() > 2)
            {
                for(int i=0; i<intersections.size(); i++)
                {
                    for(int j=i+1; j<intersections.size(); j++)
                    {
                        if(D2(intersections[i],intersections[j]) < 0.1)
                        {
                            intersections.erase(intersections.begin()+j);
                            break;
                        }
                    }
                }
            }

            if(intersections.size()>=2)
            {
                if(D2(intersections[0],lm.back()) < D2(intersections[1],lm.back()))
                {
                    lm.push_back(intersections[0]);
                    lm.push_back(intersections[1]);
                }
                else
                {
                    lm.push_back(intersections[1]);
                    lm.push_back(intersections[0]);
                }
            }
        }

      }

    if(lm.size() > 2)
    {
        lm.erase(lm.begin());
        lm.erase(lm.begin());
    }
    else
    {
        lm[0] -= ((offset0-cellW/2)-height/2) * sweepDir;
        lm[1] -= ((offset0-cellW/2)-height/2) * sweepDir;
    }

 }

void TargetPolygon::glDraw()
{
    if(ch.size()<=1)
        return;


    //glColor3f(ch[0]->colorBasis[0], ch[0]->colorBasis[1], ch[0]->colorBasis[2]);
    glColor3f(1,1,1);
    glLineWidth(4);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_POLYGON);

    for(unsigned int i=0; i<ch.size();i++)
    {
        //glColor3f(RAND((i%5)/0.5,(i%5)/0.5+0.2),RAND(0,1),RAND(0,1));
        TooN::Vector<3> p1 = ch[i]->GetMAVWaypoint();
        glVertex3f(p1[0],p1[1],p1[2]);
    }
    glEnd();

    if(base_idx[0] != base_idx[1])
    {
        glColor3f(1,0,0);
        glPointSize(5);
        glBegin(GL_POINTS);
        TooN::Vector<3> p1 = ch[base_idx[0]]->GetMAVWaypoint();
        TooN::Vector<3> p2 = ch[base_idx[1]]->GetMAVWaypoint();
        glColor3f(1,0,0);
        glVertex3f(p1[0],p1[1],p1[2]);
        glColor3f(0,1,0);
        glVertex3f(p2[0],p2[1],p2[2]);
        glEnd();

        if(lm.size() > 1)
        {
            glColor3f(0.5,0.5,1);
            //glLineWidth(6);
            //glBegin(GL_LINES);
            glPointSize(8);
            glBegin(GL_POINTS);
            for(int i=0; i<lm.size()-1; i+=1)
            {
               TooN::Vector<3> p1 = lm[i];
               TooN::Vector<3> p2 = lm[i+1];
               glVertex3f(p1[0], p1[1], p1[2]+0.5);
               glVertex3f(p2[0], p2[1], p2[2]+0.5);
            }
            glEnd();
        }
    }
}