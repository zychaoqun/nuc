#include "CNode.h"
#include <math.h>
#include <GL/glut.h>
#include <algorithm>
#include "NUCParam.h"
#include "ScalarField.h"

#define IN(x,y)    (y[0] <= x[0] && x[0] <= y[2] && y[1] <= x[1] && x[1] <= y[3])
#define D2_OF3(a,b) sqrt((a[0]-b[0])*(a[0]-b[0])+(a[1]-b[1])*(a[1]-b[1]))

#define nuc_alpha(h,mh)   (NUCParam::alpha_h0 + (h/mh)*(NUCParam::alpha_hm-NUCParam::alpha_h0))
#define nuc_beta(h,mh)    (NUCParam::beta_h0 + (h/mh)*(NUCParam::beta_hm-NUCParam::beta_h0))

#define PRIOR_INTERESTING 0.5
#define PRIOR_UNINTERESTING 0.3
#define INTERESTING_THRESHOLD   NUCParam::int_prob_thr

bool CNode::drawEdges = false;
bool CNode::drawCoverage = false;
double CNode::rootHeight = 0;
int CNode::maxDepth = 0;
double CNode::int_thr[20];
double CNode::cut_off;
CNode::DrawingMode CNode::drawing_mode=CNode::DrawingMode::gp_true_f;

CNode::CNode(Rect target_foot_print, bool populateChildren):parent(NULL)
{
    searchParentNode = NULL;
    mav_wp_cached = false;
    searchNode = false;
    ancestor_visited = false;
    label = -1;
    extra_info = false;
    colorBasis = TooN::makeVector(255.0/255.0, 249.0/255.0, 214.0/255.0);
    nextHilbertLeaf = NULL;
    gnode = NULL;
    coverage = 0;
    isInterestingnessSet = false;
    visited = false;
    grd_x = 0;
    grd_y = 0;
    depth = 0;
    isInteresting = false;
    trueIsInteresting = false;
    footPrint = target_foot_print;
    pos[0] = (0.5)*(footPrint[0]+footPrint[2]);
    pos[1] = (0.5)*(footPrint[1]+footPrint[3]);
    pos[2] = (0.5)*fabs((footPrint[0]-footPrint[2])/tan(NUCParam::FOV/2.0));
    p_X = RAND(0.01,PRIOR_UNINTERESTING);

    if(rootHeight < pos[2])
        rootHeight = pos[2];

    if(populateChildren)
        PopulateChildren();

    p1[0] = footPrint[0];
    p1[1] = footPrint[1];
    p2[0] = footPrint[2];
    p2[1] = footPrint[3];

    p3 = TooN::makeVector(p1[0], p2[1]);
    p4 = TooN::makeVector(p2[0], p1[1]);

    v1 = Rotation2D(p1, NUCParam::area_rotation,TooN::makeVector(NUCParam::cx, NUCParam::cy));
    v2 = Rotation2D(p3, NUCParam::area_rotation,TooN::makeVector(NUCParam::cx, NUCParam::cy));
    v3 = Rotation2D(p2, NUCParam::area_rotation,TooN::makeVector(NUCParam::cx, NUCParam::cy));
    v4 = Rotation2D(p4, NUCParam::area_rotation,TooN::makeVector(NUCParam::cx, NUCParam::cy));

}

CNode::~CNode()
{
    while(!children.empty())
    {
        CNode* n = children.back();
        children.pop_back();

        delete n;
    }

    //delete gnode;
}

CNode * CNode::CreateChildNode(Rect fp)
{
    CNode* cnode = new CNode(fp);
    cnode->parent = this;
    cnode->depth = depth +1;
    children.push_back(cnode);
    return cnode;
}

double CNode::GetLocalPrior()
{
    double r = NUCParam::prob_r;

    if(depth < maxDepth-2)
        return p_X;

    //ROS_INFO_THROTTLE(1,"%d", maxDepth);

    r = r + (1-r)*((double)(maxDepth - depth))/maxDepth;
    //if(depth != maxDepth)
    //    return p_X;

    //ROS_INFO("**** Depth: %d %c ****", depth, trueIsInteresting?'T':'F');

    //ROS_INFO("r: %.2f", r);

    double p_val = p_X;
    double nval = 0;
    int c=0;

    if(!neighbours_populated)
    {
        nds[0] = GetNeighbourLeaf(true, false, false, false);
        nds[1] = GetNeighbourLeaf(false, true, false, false);
        nds[2] = GetNeighbourLeaf(false, false, true, false);
        nds[3] = GetNeighbourLeaf(false, false, false, true);

        if(nds[0])
        {
            nds[4] = nds[0]->GetNeighbourLeaf(false, false, true, false);
        }
        else if(nds[2])
        {
            nds[4] = nds[2]->GetNeighbourLeaf(true, false, false, false);
        }

        if(nds[0])
        {
            nds[5] = nds[0]->GetNeighbourLeaf(false, false, false, true);
        }
        else if(nds[3])
        {
            nds[5] = nds[3]->GetNeighbourLeaf(true, false, false, false);
        }

        if(nds[1])
        {
            nds[6] = nds[1]->GetNeighbourLeaf(false, false, true, false);
        }
        else if(nds[2])
        {
            nds[6] = nds[2]->GetNeighbourLeaf(false, true, false, false);
        }

        if(nds[1])
        {
            nds[7] = nds[1]->GetNeighbourLeaf(false, false, false, true);
        }
        else if(nds[3])
        {
            nds[7] = nds[3]->GetNeighbourLeaf(false, true, false, false);
        }

        neighbours_populated = true;
    }

    for(unsigned int i=0; i<8; i++)
    {
        if(nds[i]!=NULL && fabs(nds[i]->p_X-0.5) > 0.1 )
        {
            //ROS_INFO("p%d: %.2f",c, nds[i]->p_X);
            nval += nds[i]->p_X;
            c++;
        }
    }

    if(c > 0)
    {
        nval /= c;
    }

   // c=8;

    //ROS_INFO("p_X: %.2f", p_X);
    p_val = ((r+(1-r)*((8.0-c)/8.0))*p_X) + (1.0-r)*(c/8.0)*nval;

    //ROS_INFO("new_p_X: %.2f", p_val);
    //ROS_INFO("**** End ****");

    return p_val;
}

double CNode::CoverageReward()
{
    // implement some reward function. (expected number of targets beeing sensed with high resolution?)
    //return p_X*0.5*(1-log(0.5+0.95*((double)depth)/maxDepth)/log(2));
    return p_X;
    //return p_X*(1/(1+20*((double)depth)/maxDepth));
    //    if(IsLeaf())
    //        return p_X;
    //    else
    //        return 0.0;
}

bool CNode::IsNodeInteresting()
{

    //return p_X >= INTERESTING_THRESHOLD/(1.0*depth);
    if(IsLeaf())
    {
        //double p_val = GetLocalPrior();
        return p_X >= INTERESTING_THRESHOLD;
    }
    else
    {
        for(unsigned int i=0; i < this->children.size(); i++)
            if(this->children[i]->IsNodeInteresting())
                return true;
        return false;
    }
}

void CNode::PopulateChildren()
{
    double fps = (footPrint[2]-footPrint[0]);
    if(fps <= NUCParam::min_footprint)
    {
        ROS_INFO_ONCE("leaf: fp:%f height:%f", fps, pos[2]);
        return;
    }

    double dl = fps/NUCParam::bf_sqrt;

    for(int i=0; i<NUCParam::bf_sqrt; i++)
        for(int j=0; j<NUCParam::bf_sqrt; j++)
        {
            Rect fp;
            fp[0] = footPrint[0]+i*dl;
            fp[1] = footPrint[1]+j*dl;
            fp[2] = fp[0]+dl;
            fp[3] = fp[1]+dl;

            CNode* ch = CreateChildNode(fp);

            /*
             *  (0,2)   (1,2)   (2,2)
             *  (0,1)   (1,1)   (2,1)

     *  (0,0)   (1,0)   (2,0)
             */
            ch->grd_x = i;
            ch->grd_y = j;

        }
}

TooN::Vector<3> CNode::GetMAVWaypoint()
{
    //if(!mav_wp_cached)
    {
        TooN::Vector<2> c = TooN::makeVector(NUCParam::cx, NUCParam::cy);
        TooN::Matrix<2,2,double> rot = TooN::Data(cos(NUCParam::area_rotation*D2R), sin(NUCParam::area_rotation*D2R),
                                         -sin(NUCParam::area_rotation*D2R), cos(NUCParam::area_rotation*D2R));
        TooN::Vector<2> v = TooN::makeVector(pos[0],pos[1]);
        v = c+rot*(v-c);

        mav_wp_cached = true;
        mav_wp = TooN::makeVector(v[0], v[1], pos[2]);
    }

    return mav_wp;
}

TooN::Vector<2> CNode::Rotation2D(TooN::Vector<2> v, double deg, TooN::Vector<2> c)
{
    TooN::Matrix<2,2,double> rot = TooN::Data(cos(deg*D2R), sin(deg*D2R),
                                     -sin(deg*D2R), cos(deg*D2R));
    return (c+ rot*(v-c));
}

TooN::Vector<3> CNode::Rotation2D(TooN::Vector<3> v, double deg, TooN::Vector<2> c)
{
    TooN::Matrix<2,2,double> rot = TooN::Data(cos(deg*D2R), sin(deg*D2R),
                                     -sin(deg*D2R), cos(deg*D2R));

    TooN::Vector<2> v2 = TooN::makeVector(v[0],v[1]);
    v2 = c+rot*(v2-c);

    return TooN::makeVector(v2[0], v2[1], v[2]);
}

double CNode::Cost(CNode *from, CNode *to)
{
    return sqrt((from->pos-to->pos)*(from->pos-to->pos));
}

void CNode::PopulateInt_Thr(int maxdepth)
{
    maxDepth = maxdepth;

    int_thr[maxDepth] = INTERESTING_THRESHOLD;

    for(int i=maxDepth-1; i >= 0; i--)
    {
        int_thr[i] = 1-(1-int_thr[i+1])*(1-int_thr[i+1])*(1-int_thr[i+1])*(1-int_thr[i+1]);
        ROS_INFO("THR Depth:%d  %f", i, int_thr[i]);
    }


}

void CNode::PrintoutParams()
{
    float al = nuc_alpha(pos[2],rootHeight);
    float be = nuc_beta(pos[2],rootHeight);
    ROS_INFO("D:%d \t H:%.2f \t alpha:%.2f \t beta:%.2f", depth, pos[2], al, be);
    if(!IsLeaf())
        children[0]->PrintoutParams();
}

void CNode::glDraw()
{
    if(parent != NULL && drawEdges)
    {
        TooN::Vector<3> v2 = Rotation2D(pos, NUCParam::area_rotation, TooN::makeVector(NUCParam::cx, NUCParam::cy));
        glLineWidth(2);
        glColor4f(.1,.1,.1,0.3);
        glBegin(GL_LINES);
        TooN::Vector<3> v1 = Rotation2D(parent->pos, NUCParam::area_rotation, TooN::makeVector(NUCParam::cx, NUCParam::cy));

        glVertex3f(v1[0],v1[1],v1[2]);
        glVertex3f(v2[0],v2[1],v2[2]);
        glEnd();
    }

    double df = 0.05;

    for(unsigned int i=0; i<children.size(); i++)
        children[i]->glDraw();

        if(IsLeaf())
        {
            glLineWidth(1);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            if(drawing_mode == DrawingMode::gp_f)
            {
                glColor4f(ceil(gp_value/df)*df,ceil(gp_value/df)*df,ceil(gp_value/df)*df,1);
            }
            else if(drawing_mode == DrawingMode::gp_var)
            {
                glColor4f(gp_var,gp_var,gp_var,1);
            }
            else if(drawing_mode == DrawingMode::gp_f_var)
            {
                glColor4f(gp_var,((1-gp_var)<0?0:(1-gp_var))*gp_value,gp_var,1);
            }
            else if(drawing_mode == DrawingMode::gp_true_f)
            {
                glColor4f(imgPrior,imgPrior,imgPrior,1);
            }
            else if(!visited)
            {
                TooN::Vector<3> cl = TooN::makeVector(0,0,0);//colorBasis;
                glColor4f(cl[0],cl[1],cl[2],1);

                if(p_X >= 0.5)
                {
                    TooN::Vector<3> cl = colorBasis;
                    glColor4f(cl[0],cl[1],cl[2],1);
                }
                else
                {
                    TooN::Vector<3> cl = TooN::makeVector(254.0/255.0,254.0/255.0,250.0/255.0);//p_X*colorBasis;
                    glColor4f(cl[0],cl[1],cl[2],1);
                }
            }
            else
            {
                TooN::Vector<3> cl = p_X*colorBasis;
                glColor4f(cl[0],cl[1],cl[2],1);
            }

//            glBegin(GL_POLYGON);
//            glVertex3f(v1[0],v1[1], (0.6-depth/20.0));
//            glVertex3f(v2[0],v2[1], (0.6-depth/20.0));
//            glVertex3f(v3[0],v3[1], (0.6-depth/20.0));
//            glVertex3f(v4[0],v4[1], (0.6-depth/20.0));
//            glVertex3f(v1[0],v1[1], (0.6-depth/20.0));
//            glEnd();
            glBegin(GL_QUADS);
            glVertex3f(v1[0],v1[1], (0.6-depth/20.0));
            glVertex3f(v2[0],v2[1], (0.6-depth/20.0));
            glVertex3f(v3[0],v3[1], (0.6-depth/20.0));
            glVertex3f(v4[0],v4[1], (0.6-depth/20.0));
            glEnd();

        }

        if(searchNode)
        {
            glColor3f(0.8,0.8,0.8);
            glLineWidth(1);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glBegin(GL_QUADS);
            glVertex3f(v1[0],v1[1], (0.6-depth/20.0));
            glVertex3f(v2[0],v2[1], (0.6-depth/20.0));
            glVertex3f(v3[0],v3[1], (0.6-depth/20.0));
            glVertex3f(v4[0],v4[1], (0.6-depth/20.0));
            glEnd();
        }

}

void CNode::PropagateDepth()
{
    if(parent == NULL)
        depth = 0;
    else
        depth = parent->depth +1;

    if(maxDepth < depth)
        maxDepth = depth;

    for(unsigned int i=0; i<children.size(); i++)
    {
        children[i]->PropagateDepth();
    }
}

void CNode::GenerateTargets(double prob_cutoff)
{
    cut_off = prob_cutoff;
    if(IsLeaf())
    {
        p_X = (gp_value < prob_cutoff)?0.0:1.0;
        //p_X = (imgPrior < prob_cutoff)?0.0:1.0;
    }
    else
    {
        for(unsigned int i=0; i<children.size(); i++)
        {
            children[i]->GenerateTargets(prob_cutoff);
        }
    }
}

void CNode::UpdateGPValues()
{
    if(IsLeaf())
    {
        double x[]={pos[0], pos[1]};
        gp_value = ScalarField::GetInstance()->f(x);
        gp_var  = ScalarField::GetInstance()->var(x);
    }
    else
    {
        for(auto &c: children)
            c->UpdateGPValues();
    }
}

void CNode::SetTreeVisited(bool visited_)
{
    this->visited = visited_;
    for(unsigned int i=0; i<children.size(); i++)
    {
        children[i]->SetTreeVisited(visited_);
    }
}

void CNode::SetAncestorVisited(bool av)
{
    this->ancestor_visited = av;
    for(unsigned int i=0; i<children.size(); i++)
    {
        children[i]->SetAncestorVisited(av);
    }
}

void CNode::AddDependentPolygon(TargetPolygon *tp)
{
    if(std::find(dependents.begin(), dependents.end(), tp) == dependents.end())
    {
        dependents.push_back(tp);
    }
}

void CNode::RemoveDependentPolygon(TargetPolygon *tp)
{
    std::vector<TargetPolygon*>::iterator it = std::find(dependents.begin(), dependents.end(), tp);
    if( it != dependents.end())
    {
        dependents.erase(it);
    }
}

bool CNode::PropagateInterestingness(Rect r)
{
    if(children.empty())
    {
        if(trueIsInteresting)
            return true;

        trueIsInteresting = IN(pos,r);

        return trueIsInteresting;
    }

    bool flag = false;
    for(unsigned int i=0; i<children.size(); i++)
    {
        bool res = children[i]->PropagateInterestingness(r);
        flag  = flag || res;
    }

    trueIsInteresting = trueIsInteresting || flag;

    return trueIsInteresting;
}

double CNode::InitializePrior(Rect r)
{
    if(children.empty())
    {
        if(IN(pos,r))
        {
            p_X = RAND(PRIOR_INTERESTING,0.99);//PRIOR_INTERESTING;
        }

        return p_X;
    }

    double prod = 1;
    for(unsigned int i=0; i<children.size(); i++)
    {
        prod *= (1-children[i]->InitializePrior(r));
    }

    p_X = 1-prod;

    assert(p_X <= 1);
    assert(p_X >= 0);

    return p_X;
}

bool CNode::VisitedInterestingDescendentExists()
{
    if(children.empty())
        return (visited && IsNodeInteresting());

    bool flag = false;
    for(unsigned int i=0; i<children.size(); i++)
    {
        flag = children[i]->VisitedInterestingDescendentExists();
        if(flag)
            return true;
    }

    return false;
}

bool CNode::IsNeighbour(CNode *n)
{
    if(searchNode && n->searchNode)
    {
        if((GetMAVWaypoint()-n->GetMAVWaypoint())*(GetMAVWaypoint()-n->GetMAVWaypoint()) - (footPrint[0]-footPrint[2])*(footPrint[0]-footPrint[2]) < 0.2 )
            return true;
    }
    else
    {
        for(int i=1; i<=4; i++)
            if(n == GetNeighbourLeaf(i==1, i==2, i==3, i==4, maxDepth-depth))
                return true;
    }

    return false;
}

void CNode::GetLeavesInRange(std::set<CNode*> &list, double range, TooN::Vector<3> center)
{
    double r = 1.4142*fabs(footPrint[0]-footPrint[2]);
    if(D2_OF3(GetMAVWaypoint(), center) < r+range)
    {
        if(IsLeaf())
        {
            list.insert(this);
        }
        else
        {
            for(size_t i=0; i<children.size(); i++)
                children[i]->GetLeavesInRange(list, range, center);
        }
    }

    return;
}

CNode* CNode::GetNeighbourLeaf(bool left, bool right, bool up, bool down, int inverse_depth)
{
    TooN::Vector<3> queryVec;
    TooN::Vector<3> shiftVec;

    if(left)
    {
        shiftVec = fabs(footPrint[0]-footPrint[2]) * TooN::makeVector(-1,0,0);
    }
    else if(right)
    {
        shiftVec = fabs(footPrint[0]-footPrint[2]) * TooN::makeVector(1,0,0);
    }
    else if(up)
    {
        shiftVec = fabs(footPrint[1]-footPrint[3]) * TooN::makeVector(0,1,0);
    }
    else if(down)
    {
        shiftVec = fabs(footPrint[1]-footPrint[3]) * TooN::makeVector(0,-1,0);
    }

    queryVec = pos + shiftVec;

    CNode* root = this;

    while(root->parent)
        root = root->parent;

    CNode *n = root->GetNearestLeaf(queryVec,inverse_depth);

    if(n)
    {
        if(!IN(queryVec, n->footPrint))
        {
            n = NULL;
           // ROS_INFO("Not in rect");
        }
        else
        {

        }
    }

    return n;
}

CNode* CNode::GetNearestNode(TooN::Vector<3> p) // can be optimized
{
    if(children.empty())
        return this;

    int minidx=0;
    double minDist = 99999999999;
    std::vector<CNode *> list;
    for(unsigned int i=0; i<children.size(); i++)
    {
        list.push_back(children[i]->GetNearestNode(p));
    }

    for(unsigned int i=0; i<list.size(); i++)
    {
        double dist = (list[i]->pos-p)*(list[i]->pos-p);
        if(dist < minDist)
        {
            minidx = i;
            minDist = dist;
        }
    }

    if(minDist > (pos-p)*(pos-p))
        return this;
    else
        return list[minidx];
}

CNode* CNode::GetNearestLeaf(TooN::Vector<3> p, int revDepth)
{
 //   ROS_INFO("revDepth: %d  maxDepth: %d depth: %d", revDepth, maxDepth, depth);

    if(children.empty() || (maxDepth-revDepth)==depth)
        return this;

    int minidx=0;
    double minDist = 99999999999;
    for(unsigned int i=0; i<children.size(); i++)
    {
        double dist = (children[i]->pos-p)*(children[i]->pos-p);
        if(dist < minDist)
        {
            minidx = i;
            minDist = dist;
        }
    }

    return children[minidx]->GetNearestLeaf(p, revDepth);
}

void CNode::GetNearestLeafAndParents(TooN::Vector<3> p, std::vector<CNode*> & list)
{
    if(children.empty())
        return;

    int minidx=0;
    double minDist = 99999999999;
    for(unsigned int i=0; i<children.size(); i++)
    {
        double dist = (children[i]->pos-p)*(children[i]->pos-p);
        if(dist < minDist)
        {
            minidx = i;
            minDist = dist;
        }
    }

    list.push_back(children[minidx]);
    return children[minidx]->GetNearestLeafAndParents(p, list);
}

void CNode::GenerateObservationAndPropagate()
{
    if(!IsLeaf())
    {
        bool obs[2][2];

        for(unsigned int i=0; i<children.size();i++)
        {
            bool observation = false;
            if(children[i]->trueIsInteresting)
            {
                observation = (RAND(0,1) < nuc_beta(pos[2],rootHeight)) ? false : true;
            }
            else
            {
                observation = (RAND(0,1) < nuc_alpha(pos[2],rootHeight)) ? true : false;
            }

            obs[children[i]->grd_x][children[i]->grd_y] = observation;
        }

        for(unsigned int i=0; i<children.size();i++)
        {
            children[i]->PropagateObservation(obs[children[i]->grd_x][children[i]->grd_y]);
        }

        this->RecomputeProbability();

        //for(unsigned int i=0; i<children.size(); i++)
        //    ROS_INFO("%d %d %.2f", children[i]->grd_x, children[i]->grd_y, children[i]->p_X);

        //ROS_INFO("p_X: %.2f", p_X);

    }
    else
    {
        bool X=trueIsInteresting;

//        if(trueIsInteresting)
//        {
//            X = (RAND(0,1) < nuc_beta(pos[2],rootHeight)) ? false : true;
//        }
//        else
//        {
//            X = (RAND(0,1) < nuc_alpha(pos[2],rootHeight)) ? true : false;
//        }

        this->PropagateObservation(X);
        parent->RecomputeProbability();
        //ROS_INFO("p_X: %.2f", p_X);
    }
}

void CNode::RecomputeProbability()
{
    double prod = 1;
    for(unsigned int i=0; i<children.size();i++)
    {
        prod *= (1-children[i]->p_X);
    }

    p_X = 1-prod;

    assert(p_X>=0.0);
    assert(p_X<=1.0);

    if(parent)
        parent->RecomputeProbability();
}

double CNode::RecomputePrior()
{
    if(IsLeaf())
        return p_X;

    double prod = 1;
    for(unsigned int i=0; i<children.size();i++)
    {
        prod *= (1-children[i]->RecomputePrior());
    }

    p_X = 1-prod;

    assert(p_X>=0.0);
    assert(p_X<=1.0);

    return p_X;
}

void CNode::PropagateObservation(bool X)
{
    //double p_X_1 = GetLocalPrior();// p_X;
    double p_X_1 = p_X;
    //ROS_INFO("Prior_X: %.2f", p_X_1);

    double new_p_X;

    if(X)
    {
        new_p_X = (p_X_1 * (1-nuc_beta(pos[2],rootHeight)))/(p_X_1*(1-nuc_beta(pos[2],rootHeight)) + (1-p_X_1)*nuc_alpha(pos[2],rootHeight));
    }
    else
    {
        new_p_X = (p_X_1 * nuc_beta(pos[2],rootHeight))/(p_X_1*nuc_beta(pos[2],rootHeight) + (1-p_X_1)*(1-nuc_alpha(pos[2],rootHeight)));
    }

    UpdateProbability(new_p_X);
}

void CNode::UpdateProbability(double new_p_X)
{

    double a = 0;
    //ROS_INFO("P_X_NEW: %.2f", new_p_X);

    if(fabs(new_p_X-p_X) > 0.005 && fabs(1-p_X) > 0.01)
    {

        a = sqrt(sqrt((1-new_p_X)/(1-p_X)));

        for(unsigned int i=0; i<children.size();i++)
        {
            double px = children[i]->p_X;
            double newpx = 1- a*(1-px);
            newpx = newpx < 0.0 ? 0.0: newpx;
            //ROS_INFO("child: a: %.2f p_X: %.2f new_p_X: %.2f", a, px, newpx);
            //children[i]->UpdateProbability(newpx);
        }
    }
    else
    {
        //ROS_INFO("Not too much change!");
    }

    //ROS_INFO("P_X: %.2f", p_X);

    assert(new_p_X>=0.0);
    assert(new_p_X<=1.0);



    if(fabs(new_p_X-p_X) > 0.005  && fabs(1-p_X) > 0.01)
    {
        for(unsigned int i=0; i<children.size();i++)
        {
            double px = children[i]->p_X;
            double newpx = 1- a*(1-px);
            newpx = newpx < 0.0 ? 0.0: newpx;
            //ROS_INFO("child: a: %.2f p_X: %.2f new_p_X: %.2f", a, px, newpx);
            children[i]->UpdateProbability(newpx);
        }
    }

    p_X = new_p_X;

   // ROS_INFO("P_X: %.2f", p_X);
}
int CNode::ComputeDepth(int & d)
{
    d++;
    if(!IsLeaf())
    {
        children[0]->ComputeDepth(d);
    }

    return d;
}

void CNode::propagateCoverage(double height)
{
    //double newcoverage = 1 - (height/rootHeight);
    //coverage = (newcoverage > coverage) ? newcoverage :coverage;
    coverage +=1.0;
    for(unsigned int i=0; i<children.size();i++)
        children[i]->propagateCoverage(height);
}

bool CNode::ChildrenNeedVisitation()
{
    for(unsigned int i=0; i<children.size(); i++)
        if(children[i]->NeedsVisitation())
            return true;

    return false;
}
bool CNode::ChildrenVisited()
{
    if(IsLeaf())
    {
        return visited;
    }
    else
    {
        if(visited)
        {
            return true;
        }
        else
        {
            for(unsigned int i=0; i<children.size(); i++)
                if(!children[i]->ChildrenVisited())
                    return false;

            return true;
        }
    }
}

void CNode::GetUnvisitedFalseNegatives(int &n)
{
    if(IsLeaf())
    {
        if(trueIsInteresting && !visited)
            n++;
    }
    else
    {
        for(unsigned int i=0; i<children.size(); i++)
        {
            children[i]->GetUnvisitedFalseNegatives(n);
        }
    }
}

bool CNode::NeedsVisitation()
{
    if(ChildrenVisited())
        return false;

    CNode * par = parent;
    while(par!=NULL)
    {
        if(!parent->IsNodeInteresting())
            return false;
        par = par->parent;
    }

    if(IsLeaf())
    {
        return !visited && IsNodeInteresting();
    }
    else
    {
        if(ChildrenNeedVisitation())
            return true;
        else
            return !ChildrenVisited() && !visited && IsNodeInteresting();
    }
}
