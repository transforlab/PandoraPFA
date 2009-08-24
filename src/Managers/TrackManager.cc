/**
 *	@file	PandoraPFANew/src/Managers/TrackManager.cc
 * 
 *	@brief	Implementation of the track manager class.
 * 
 *	$Log: $
 */

#include "Managers/TrackManager.h"

#include "Objects/Track.h"

#include <sstream>

namespace pandora
{

const std::string TrackManager::INPUT_LIST_NAME = "input";

//------------------------------------------------------------------------------------------------------------------------------------------

TrackManager::~TrackManager()
{
	(void) this->ResetForNextEvent();
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::CreateTrack(const PandoraApi::TrackParameters &trackParameters)
{
	Track *pTrack = NULL;
	pTrack = new Track(trackParameters);

	if (NULL == pTrack)
		return STATUS_CODE_FAILURE;

	NameToTrackListMap::iterator iter = m_nameToTrackListMap.find(INPUT_LIST_NAME);

	if (m_nameToTrackListMap.end() == iter)
	{
		m_nameToTrackListMap[INPUT_LIST_NAME] = new TrackList;
		m_savedLists.insert(INPUT_LIST_NAME);
		iter = m_nameToTrackListMap.find(INPUT_LIST_NAME);
		
		if (m_nameToTrackListMap.end() == iter)
			return STATUS_CODE_FAILURE;
	}

	iter->second->insert(pTrack);

	return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::GetList(const std::string &listName, const TrackList *pTrackList) const
{
	NameToTrackListMap::const_iterator iter = m_nameToTrackListMap.find(listName);
	
	if (m_nameToTrackListMap.end() == iter)
		return STATUS_CODE_NOT_INITIALIZED;
	
	pTrackList = iter->second;

	return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::ReplaceCurrentAndAlgorithmInputLists(const Algorithm *const pAlgorithm, const std::string &trackListName)
{
	if (m_nameToTrackListMap.end() == m_nameToTrackListMap.find(trackListName))
		return STATUS_CODE_NOT_FOUND;

	if ((m_algorithmInfoMap.size() > 1) || (m_savedLists.end() == m_savedLists.find(trackListName)))
		return STATUS_CODE_NOT_ALLOWED;

	m_currentListName = trackListName;

	AlgorithmInfoMap::iterator iter = m_algorithmInfoMap.find(pAlgorithm);	

	if (m_algorithmInfoMap.end() == iter)
		return STATUS_CODE_NOT_FOUND;

	iter->second.m_parentListName = trackListName;

	return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::CreateTemporaryListAndSetCurrent(const Algorithm *const pAlgorithm, const TrackList &trackList,
	std::string &temporaryListName)
{
	if (trackList.empty())
		return STATUS_CODE_NOT_INITIALIZED;

	std::ostringstream temporaryListNameStream;
	temporaryListNameStream << pAlgorithm;

	AlgorithmInfoMap::iterator iter = m_algorithmInfoMap.find(pAlgorithm);	

	if (m_algorithmInfoMap.end() == iter)
		return STATUS_CODE_NOT_FOUND;

	temporaryListNameStream << "_" << iter->second.m_temporaryListNames.size();

	iter->second.m_temporaryListNames.insert(temporaryListNameStream.str());
	temporaryListName = temporaryListNameStream.str();

	m_nameToTrackListMap[temporaryListName] = new TrackList(trackList);
	m_currentListName = temporaryListName;	

	return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::SaveTemporaryList(const Algorithm *const pAlgorithm, const std::string &newListName, const std::string &temporaryListName)
{
	if (m_nameToTrackListMap.end() != m_nameToTrackListMap.find(newListName))
		return STATUS_CODE_NOT_ALLOWED;

	NameToTrackListMap::iterator iter = m_nameToTrackListMap.find(temporaryListName);

	if (m_nameToTrackListMap.end() == iter)
		return STATUS_CODE_NOT_FOUND;

	if (iter->second->empty())
		return STATUS_CODE_NOT_INITIALIZED;

	if (!m_nameToTrackListMap.insert(NameToTrackListMap::value_type(newListName, new TrackList)).second)
		return STATUS_CODE_FAILURE;

	*(m_nameToTrackListMap[newListName]) = *(iter->second);
	m_savedLists.insert(newListName);	

	PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, RemoveTemporaryList(pAlgorithm, temporaryListName));

	return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::SaveList(const TrackList &trackList, const std::string &newListName)
{
	if (m_nameToTrackListMap.end() != m_nameToTrackListMap.find(newListName))
		return STATUS_CODE_FAILURE;

	if (!m_nameToTrackListMap.insert(NameToTrackListMap::value_type(newListName, new TrackList)).second)
		return STATUS_CODE_FAILURE;

	*(m_nameToTrackListMap[newListName]) = trackList;
	m_savedLists.insert(newListName);

	return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::RegisterAlgorithm(const Algorithm *const pAlgorithm)
{
	if (m_algorithmInfoMap.end() != m_algorithmInfoMap.find(pAlgorithm))
		return STATUS_CODE_ALREADY_INITIALIZED;
	
	AlgorithmInfo algorithmInfo;
	algorithmInfo.m_parentListName = m_currentListName;

	if (!m_algorithmInfoMap.insert(AlgorithmInfoMap::value_type(pAlgorithm, algorithmInfo)).second)
		return STATUS_CODE_FAILURE;

	return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::ResetAfterAlgorithmCompletion(const Algorithm *const pAlgorithm)
{
	AlgorithmInfoMap::iterator algorithmListIter = m_algorithmInfoMap.find(pAlgorithm);
	
	if (m_algorithmInfoMap.end() == algorithmListIter)
		return STATUS_CODE_NOT_FOUND;

	for (StringSet::const_iterator listNameIter = algorithmListIter->second.m_temporaryListNames.begin(),
		listNameIterEnd = algorithmListIter->second.m_temporaryListNames.end(); listNameIter != listNameIterEnd; ++listNameIter)
	{
		NameToTrackListMap::iterator iter = m_nameToTrackListMap.find(*listNameIter);
		
		if (m_nameToTrackListMap.end() == iter)
			return STATUS_CODE_FAILURE;
		
		m_nameToTrackListMap.erase(iter);
	}

	m_currentListName = algorithmListIter->second.m_parentListName;
	m_algorithmInfoMap.erase(algorithmListIter);

	return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::ResetForNextEvent()
{
	NameToTrackListMap::iterator inputIter = m_nameToTrackListMap.find(INPUT_LIST_NAME);

	if (m_nameToTrackListMap.end() != inputIter)
	{
		for (TrackList::iterator inputTrackIter = inputIter->second->begin(), inputTrackIterEnd = inputIter->second->end(); 
			inputTrackIter != inputTrackIterEnd; ++inputTrackIter)
		{
			delete *inputTrackIter;
		}
	}

	for (NameToTrackListMap::iterator iter = m_nameToTrackListMap.begin(), iterEnd = m_nameToTrackListMap.end(); iter != iterEnd; ++iter)
	{
		delete iter->second;
		m_nameToTrackListMap.erase(iter);
	}

	m_nameToTrackListMap.clear();
	m_savedLists.clear();
	m_currentListName.clear();

	return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackManager::RemoveTemporaryList(const Algorithm *const pAlgorithm, const std::string &temporaryListName)
{
	m_nameToTrackListMap.erase(m_nameToTrackListMap.find(temporaryListName));

	AlgorithmInfoMap::iterator algorithmListIter = m_algorithmInfoMap.find(pAlgorithm);	

	if (m_algorithmInfoMap.end() == algorithmListIter)
		return STATUS_CODE_NOT_FOUND;

	StringSet *pStringSet = &(algorithmListIter->second.m_temporaryListNames);
	pStringSet->erase(pStringSet->find(temporaryListName));

	if (temporaryListName == m_currentListName)
		m_currentListName = algorithmListIter->second.m_parentListName;

	return STATUS_CODE_SUCCESS;
}

} // namespace pandora
