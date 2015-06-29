#include "TargetTour.h"
#include <algorithm>

int TargetComp(TargetPolygon* tp1, TargetPolygon* tp2){return tp1->GetCenter()[0] > tp2->GetCenter()[0];}


double TargetTour::GetTargetTour(vector<TargetPolygon*> &targets, Vector<3> start, Vector<3> end)
{
    vector<TargetPolygon*> tmp;
    copy(targets.begin(), targets.end(), back_inserter(tmp));

    double cost = 9999999999;

    std::sort(tmp.begin(), tmp.end(), TargetComp);

    do
    {
        double pcost = GetTourCost(tmp, start, end);
        if(pcost < cost)
        {
            cost = pcost;
            targets.clear();
            copy(tmp.begin(), tmp.end(), back_inserter(targets));
        }

    }while(next_permutation(targets.begin(), targets.end()));

    return cost;
}

double TargetTour::GetTourCost(vector<TargetPolygon *> &targets, Vector<3> start, Vector<3> end)
{
    vector<Vector<3> > wps;
    wps.push_back(start);
    for(size_t i=0; i<targets.size(); i++)
    {
        wps.push_back(targets[i]->FirstLMPos());
        wps.push_back(targets[i]->LastLMPos());
    }

    wps.push_back(end);

    return GetPlanExecutionTime(wps, start, end, true, true);
}


double TargetTour::GetPlanExecutionTime(std::vector<TooN::Vector<3> > & wps, TooN::Vector<3> curpos, TooN::Vector<3> endpos, bool initialTurn, bool endTurn)
{
    if(wps.empty())
        return 0;

    double t=0;
    double dist = 0;

    dist += sqrt(D2(curpos, wps.front()));
    dist += sqrt(D2(wps.back(), endpos));

    for(size_t i=0; i+1 < wps.size(); i++)
    {
        dist += sqrt(D2(wps[i], wps[i+1]));
    }

    t = dist/NUCParam::average_speed;

    if(wps.size() >=2)
    {
        t += COLLINEAR(curpos, wps[0], wps[1]) * NUCParam::turning_time;
        for(size_t i=1; i+1 < wps.size(); i++)
        {
            t += COLLINEAR(wps[i-1], wps[i], wps[i+1]) * NUCParam::turning_time;
        }

        t += COLLINEAR(wps[wps.size()-2], wps.back(), endpos) * NUCParam::turning_time;
    }

    if(wps.size() == 1)
    {
        t += COLLINEAR(curpos, wps[0], endpos) * NUCParam::turning_time;
    }

    if(initialTurn)
        t += NUCParam::turning_time;

    if(endTurn)
        t += NUCParam::turning_time;

    return t;
}

double TargetTour::GetPlanExecutionTime(std::vector<CNode*> & wps, TooN::Vector<3> curpos, TooN::Vector<3> endpos, bool initialTurn, bool endTurn)
{
    if(wps.empty())
    {
        return sqrt(D2(curpos, endpos))/NUCParam::average_speed + NUCParam::turning_time;
    }

    double t=0;
    double dist = 0;

    //dist += sqrt(D2(curpos, wps.front()->GetMAVWaypoint()));
    dist += sqrt(D2(wps.back()->GetMAVWaypoint(), endpos));

    for(size_t i=0; i+1 < wps.size(); i++)
    {
        dist += sqrt(D2(wps[i]->GetMAVWaypoint(), wps[i+1]->GetMAVWaypoint()));
    }

    t = dist/NUCParam::average_speed;

    if(wps.size() >=2)
    {
        for(size_t i=1; i+1 < wps.size(); i++)
        {
            t += COLLINEAR(wps[i-1]->GetMAVWaypoint(), wps[i]->GetMAVWaypoint(), wps[i+1]->GetMAVWaypoint()) * NUCParam::turning_time;
        }

        t += COLLINEAR(wps[wps.size()-1]->GetMAVWaypoint(), wps.back()->GetMAVWaypoint(), endpos) * NUCParam::turning_time;
    }

    if(wps.size() == 1)
    {
        t += /*COLLINEAR(curpos, wps[0]->GetMAVWaypoint(), endpos) */ NUCParam::turning_time;
    }

    if(initialTurn)
        t += NUCParam::turning_time;

    if(endTurn)
        t += NUCParam::turning_time;

    return t;
}

double TargetTour::GetPlanExecutionTime(vector<Vector<3> > &wps, bool ignoreFirstSegment, bool ignoreLastSegment)
{
    if(wps.empty())
        return 0;

    double t=0;
    double dist = 0;

    for(size_t i=0; i+1 < wps.size(); i++)
    {
        if(ignoreFirstSegment && i==0)
            continue;

        if(ignoreLastSegment && i+2==wps.size())
            continue;

        dist += sqrt(D2(wps[i], wps[i+1]));
    }

    t = dist/NUCParam::average_speed;

    if(wps.size() >=3)
    {
        for(size_t i=0; i+2 < wps.size(); i++)
        {
            t += COLLINEAR(wps[i], wps[i+1], wps[i+2]) * NUCParam::turning_time;
        }
    }

    return t;
}