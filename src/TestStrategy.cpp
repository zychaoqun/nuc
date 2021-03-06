#include "TestStrategy.h"
#include "GL/glut.h"
#include <stdio.h>
#include "NUCParam.h"
#include "dubins.h"
#include "Trajectory.h"

using namespace TooN;

std::vector<Vector<2> > dp;

int dubin_cb(double q[], double x, void *user_data)
{
    dp.push_back(makeVector(q[0], q[1]));

    printf("%f, %f, %f, %f\n", q[0], q[1], q[2], x);
        return 0;
}


TestStrategy::TestStrategy(CNode *root)
{
    Rect r = root->GetFootPrint();
    CNode *lb = new CNode(r);
    CNode *lt = new CNode(r);
    CNode *rb = new CNode(r);
    CNode *rt = new CNode(r);

    lb->pos = makeVector(r[0], r[1], root->pos[2]);
    lt->pos = makeVector(r[0], r[3], root->pos[2]);
    rt->pos = makeVector(r[2], r[3], root->pos[2]);
    rb->pos = makeVector(r[2], r[1], root->pos[2]);

    nodeStack.push_back(lb);
    nodeStack.push_back(lt);
    nodeStack.push_back(rt);
    nodeStack.push_back(rb);
    nodeStack.push_back(lb);

   Trajectory::GenerateDubinTrajectory(makeVector(-10,-10), makeVector(10,-10), makeVector(10,-5), makeVector(-10,-5),2.5, 1, dp);
   Trajectory::GenerateDubinTrajectory(makeVector(10,-5), makeVector(-10,-5), makeVector(-10,0), makeVector(10,0), 2.5, 1, dp);
   Trajectory::GenerateDubinTrajectory(makeVector(-10,0), makeVector(10,0), makeVector(10,5), makeVector(-10,5), 2.5, 1, dp);
   Trajectory::GenerateDubinTrajectory(makeVector(10,5), makeVector(-10,5), makeVector(-10,10), makeVector(10,10), 2.5, 1, dp);
//    double l = (r[0]-r[2])*(r[0]-r[2]);
//    l = sqrt(l);
//    double ld = l;

//    int depth =0;
//    while(ld > NUCParam::min_footprint)
//    {
//        depth++;
//        ld /= CNode::bf_sqrt;
//    }

//    int n = l/ld;
//    Vector<3> startPos = root->GetNearestLeaf(makeVector(r[0],r[1],0))->GetPos();

//    printf("LM: n:%d l:%f ld:%f \n", n, l, ld);


//    for(int i=0; i< n; i++)
//        for(int j=0; j< n; j++)
//        {
//            int jj = (i%2 == 0)?j:n-j-1;
//            Vector<3> npos = startPos + makeVector(((double)i)*ld, ((double)jj)*ld, 0);
//            nodeStack.push_back(root->GetNearestLeaf(npos));
//        }
}

TestStrategy::~TestStrategy()
{

}

CNode* TestStrategy::GetNextNode()
{
    if(nodeStack.empty())
        return NULL;

    CNode* node = nodeStack.front();
    nodeStack.erase(nodeStack.begin());
    return node;
}

void TestStrategy::glDraw()
{
    glColor3f(0.0,1,0.0);
    glLineWidth(4);
    glBegin(GL_LINES);
    for(unsigned int i=1; i<dp.size();i++)
    {
        TooN::Vector<2> p1 = dp[i];
        TooN::Vector<2> p2 = dp[i-1];

        glVertex3f(p1[0],p1[1],10);
        glVertex3f(p2[0],p2[1],10);
    }
    glEnd();

    if(nodeStack.size() < 2)
        return;

    glColor3f(0.5,0.6,0.6);
    glLineWidth(4);
    glBegin(GL_LINES);
    for(unsigned int i=0; i<nodeStack.size()-1;i++)
    {
        TooN::Vector<3> p1 = nodeStack[i+1]->GetMAVWaypoint();
        TooN::Vector<3> p2 = nodeStack[i]->GetMAVWaypoint();

        glVertex3f(p1[0],p1[1],p1[2]);
        glVertex3f(p2[0],p2[1],p2[2]);
    }
    glEnd();

}
