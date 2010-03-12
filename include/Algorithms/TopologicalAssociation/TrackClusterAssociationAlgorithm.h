/**
 *  @file   PandoraPFANew/include/Algorithms/TopologicalAssociation/TrackClusterAssociationAlgorithm.h
 * 
 *  @brief  Header file for the track-cluster association algorithm class.
 * 
 *  $Log: $
 */
#ifndef TRACK_CLUSTER_ASSOCIATION_ALGORITHM_H
#define TRACK_CLUSTER_ASSOCIATION_ALGORITHM_H 1

#include "Algorithms/Algorithm.h"

/**
 *  @brief  TrackClusterAssociationAlgorithm class
 */
class TrackClusterAssociationAlgorithm : public pandora::Algorithm
{
public:
    /**
     *  @brief  Factory class for instantiating algorithm
     */
    class Factory : public pandora::AlgorithmFactory
    {
    public:
        Algorithm *CreateAlgorithm() const;
    };

private:
    StatusCode Run();
    StatusCode ReadSettings(const TiXmlHandle xmlHandle);

    float           m_lowEnergyCut;                     ///< Energy cut (GeV). Algorithm prefers to associate tracks to high-energy clusters
    float           m_maxTrackClusterDistance;          ///< Max distance between track and cluster to allow association

    unsigned int    m_maxSearchLayer;                   ///< Max pseudo layer to examine when calculating track-cluster distance
    float           m_parallelDistanceCut;              ///< Max allowed projection of track-hit separation along track direction
};

//------------------------------------------------------------------------------------------------------------------------------------------

inline pandora::Algorithm *TrackClusterAssociationAlgorithm::Factory::CreateAlgorithm() const
{
    return new TrackClusterAssociationAlgorithm();
}

#endif // #ifndef TRACK_CLUSTER_ASSOCIATION_ALGORITHM_H
