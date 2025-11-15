#include "cSongify.h"

// Michael Feeney, 2025
// mfeeney (at) fanshawec.ca
//

// Constructor sets all internal pointers and counters to a known state.
cSongify::cSongify()
	: m_pSongs(NULL)
	, m_numSongs(0)
	, m_songCapacity(0)
	, m_pHeadUser(NULL)
	, m_pTailUser(NULL)
	, m_numUsers(0)
{
}

cSongify::~cSongify()
{
	// Clean up any heap allocations we are responsible for.
	this->m_DeleteAllUsers();
	this->m_DeleteAllSongs();
}

bool cSongify::AddUser(cPerson newPerson, std::string& errorString)
{
	// Clear any previous error message.
	errorString = "";

	// Check for duplicate Songify ID.
	unsigned int newID = newPerson.getSongifyUniqueUserID();
	if (this->m_FindUserNodeBySongifyID(newID) != NULL)
	{
		errorString = "AddUser(): duplicate Songify user ID.";
		return false;
	}

	// Optional: if SIN is set, ensure there isn't already a user with this SIN.
	if (newPerson.SIN != 0)
	{
		bool moreThanOne = false;
		if (this->m_FindUserNodeBySIN(newPerson.SIN, moreThanOne) != NULL || moreThanOne)
		{
			errorString = "AddUser(): duplicate SIN.";
			return false;
		}
	}

	// Allocate and link a new user node at the tail of the list.
	sUserNode* pNewNode = new sUserNode();
	pNewNode->person = newPerson;

	if (this->m_pTailUser == NULL)
	{
		// First user in the list.
		this->m_pHeadUser = pNewNode;
		this->m_pTailUser = pNewNode;
	}
	else
	{
		this->m_pTailUser->pNext = pNewNode;
		pNewNode->pPrev = this->m_pTailUser;
		this->m_pTailUser = pNewNode;
	}

	this->m_numUsers++;
	return true;
}

// Returns false if not found
bool cSongify::FindUser(unsigned int SongifyID, cPerson& foundPerson)
{
	sUserNode* pUser = this->m_FindUserNodeBySongifyID(SongifyID);
	if (pUser == NULL)
	{
		return false;
	}

	foundPerson = pUser->person;
	return true;
}

// Returns false if not found OR there's more than one user (which shouldn't happen, anyway...)
bool cSongify::FindUserBySIN(unsigned int SIN, cPerson& foundPerson)
{
	bool moreThanOne = false;
	sUserNode* pUser = this->m_FindUserNodeBySIN(SIN, moreThanOne);

	if (pUser == NULL || moreThanOne)
	{
		return false;
	}

	foundPerson = pUser->person;
	return true;
}

bool cSongify::FindUserBySongifyID(unsigned int SongifyID, cPerson& foundPerson)
{
	return this->FindUser(SongifyID, foundPerson);
}

// This will ++OVERWRITE++ the data contained in user 
// 
// The idea is that you'd call "FindUser" then change just what needs to be
//	changed, then call this method - thereby updating what got 
// 
// NOTE: It "finds" the user by matching BOTH the "unique Songify ID" AND the "SIN" 
//        (Social Insurance Number)
// So if either of these DON'T match (or aren't found) then it reutrns false. 
// 
// Returns false if it can't find the person
bool cSongify::UpdateUser(cPerson& thePerson, std::string& errorString)
{
	// Clear any previous error message.
	errorString = "";

	unsigned int id = thePerson.getSongifyUniqueUserID();
	unsigned int sin = thePerson.SIN;

	sUserNode* pMatchingNode = NULL;
	unsigned int matchCount = 0;

	for (sUserNode* pCur = this->m_pHeadUser; pCur != NULL; pCur = pCur->pNext)
	{
		if (pCur->person.getSongifyUniqueUserID() == id &&
			pCur->person.SIN == sin)
		{
			pMatchingNode = pCur;
			matchCount++;
		}
	}

	if (matchCount != 1 || pMatchingNode == NULL)
	{
		errorString = "UpdateUser(): could not uniquely identify user.";
		return false;
	}

	// Overwrite stored user with the supplied data.
	pMatchingNode->person = thePerson;
	return true;
}

// Returns false if it can't find the person
bool cSongify::DeleteUser(unsigned int SongifyUserID, std::string& errorString)
{
	// Clear any previous error message.
	errorString = "";

	sUserNode* pUser = this->m_FindUserNodeBySongifyID(SongifyUserID);
	if (pUser == NULL)
	{
		errorString = "DeleteUser(): user not found.";
		return false;
	}

	// Unlink from list.
	if (pUser->pPrev != NULL)
	{
		pUser->pPrev->pNext = pUser->pNext;
	}
	else
	{
		// Removing head.
		this->m_pHeadUser = pUser->pNext;
	}
	if (pUser->pNext != NULL)
	{
		pUser->pNext->pPrev = pUser->pPrev;
	}
	else
	{
		// Removing tail.
		this->m_pTailUser = pUser->pPrev;
	}

	// Free this user's library.
	if (pUser->pLibrary != NULL)
	{
		delete[] pUser->pLibrary;
		pUser->pLibrary = NULL;
	}
	pUser->librarySize = 0;
	pUser->libraryCapacity = 0;

	delete pUser;
	this->m_numUsers--;
	return true;
}

// Returns false if these don't work
// 
// ---------------------------------------------------------------------
// This returns a ++COPY++ of the users library. 
// The CALLER will delete this data when done using it.
// i.e. you will have to allocate and copy the library into this array 
// ---------------------------------------------------------------------
// 
// See the cSongify.cpp file for more infomration on how this is supposed to work
// 
// It will return false if it can't find that user
// This returns a COPY of the users library, in the form of a regular dynamic array.
bool cSongify::GetUsersSongLibrary(unsigned int songifyUserID, cSong*& pLibraryArray, unsigned int& sizeOfLibary)
{
	pLibraryArray = NULL;
	sizeOfLibary = 0;

	sUserNode* pUser = this->m_FindUserNodeBySongifyID(songifyUserID);
	if (pUser == NULL)
	{
		return false;
	}

	if (pUser->librarySize == 0)
	{
		// Valid user, but empty library.
		return true;
	}

	// Allocate array of copies for the caller.
	sizeOfLibary = pUser->librarySize;
	pLibraryArray = new cSong[sizeOfLibary];

	for (unsigned int index = 0; index < sizeOfLibary; index++)
	{
		const sUserSongEntry& entry = pUser->pLibrary[index];

		// Copy base song data.
		pLibraryArray[index] = *(entry.pSong);

		// Overlay per-user stats.
		pLibraryArray[index].rating = static_cast<int>(entry.rating);
		pLibraryArray[index].numberOfTimesPlayed = static_cast<int>(entry.numPlays);
	}

	return true;
}

// Song array is sorted by this user's rating, from HIGHEST to lowest
bool cSongify::GetUsersSongLibraryAscendingBySongRating(unsigned int songifyUserID, cSong*& pLibraryArray, unsigned int& sizeOfLibary)
{
	pLibraryArray = NULL;
	sizeOfLibary = 0;

	sUserNode* pUser = this->m_FindUserNodeBySongifyID(songifyUserID);
	if (pUser == NULL)
	{
		return false;
	}

	if (pUser->librarySize == 0)
	{
		return true;
	}

	// Work on a temporary copy of the user's library.
	unsigned int count = pUser->librarySize;
	sUserSongEntry* pTemp = new sUserSongEntry[count];
	for (unsigned int i = 0; i < count; i++)
	{
		pTemp[i] = pUser->pLibrary[i];
	}

	// Insertion sort by rating (HIGHEST to lowest).
	for (unsigned int i = 1; i < count; i++)
	{
		sUserSongEntry key = pTemp[i];
		int j = static_cast<int>(i) - 1;
		while (j >= 0 && pTemp[j].rating < key.rating)
		{
			pTemp[j + 1] = pTemp[j];
			j--;
		}
		pTemp[j + 1] = key;
	}

	// Copy into output array.
	sizeOfLibary = count;
	pLibraryArray = new cSong[count];
	for (unsigned int i = 0; i < count; i++)
	{
		const sUserSongEntry& entry = pTemp[i];
		pLibraryArray[i] = *(entry.pSong);
		pLibraryArray[i].rating = static_cast<int>(entry.rating);
		pLibraryArray[i].numberOfTimesPlayed = static_cast<int>(entry.numPlays);
	}

	delete[] pTemp;
	return true;
}

// Song array is sorted by HIGHEST number of plays to lowest
bool cSongify::GetUsersSongLibraryAscendingByNumberOfPlays(unsigned int songifyUserID, cSong*& pLibraryArray, unsigned int& sizeOfLibary)
{
	pLibraryArray = NULL;
	sizeOfLibary = 0;

	sUserNode* pUser = this->m_FindUserNodeBySongifyID(songifyUserID);
	if (pUser == NULL)
	{
		return false;
	}

	if (pUser->librarySize == 0)
	{
		return true;
	}

	unsigned int count = pUser->librarySize;
	sUserSongEntry* pTemp = new sUserSongEntry[count];
	for (unsigned int i = 0; i < count; i++)
	{
		pTemp[i] = pUser->pLibrary[i];
	}

	// Insertion sort by number of plays (HIGHEST to lowest).
	for (unsigned int i = 1; i < count; i++)
	{
		sUserSongEntry key = pTemp[i];
		int j = static_cast<int>(i) - 1;
		while (j >= 0 && pTemp[j].numPlays < key.numPlays)
		{
			pTemp[j + 1] = pTemp[j];
			j--;
		}
		pTemp[j + 1] = key;
	}

	sizeOfLibary = count;
	pLibraryArray = new cSong[count];
	for (unsigned int i = 0; i < count; i++)
	{
		const sUserSongEntry& entry = pTemp[i];
		pLibraryArray[i] = *(entry.pSong);
		pLibraryArray[i].rating = static_cast<int>(entry.rating);
		pLibraryArray[i].numberOfTimesPlayed = static_cast<int>(entry.numPlays);
	}

	delete[] pTemp;
	return true;
}

// If sortAscending = true, then returns ascending sort by LAST then FIRST names
// (false returns descending)
bool cSongify::GetUsers(cPerson*& pAllTheUsers, unsigned int& sizeOfUserArray, bool sortAscending)
{
	pAllTheUsers = NULL;
	sizeOfUserArray = 0;

	if (this->m_numUsers == 0)
	{
		return true;
	}

	// Copy users into a temporary dynamic array.
	sizeOfUserArray = this->m_numUsers;
	pAllTheUsers = new cPerson[sizeOfUserArray];

	unsigned int index = 0;
	for (sUserNode* pCur = this->m_pHeadUser; pCur != NULL; pCur = pCur->pNext)
	{
		pAllTheUsers[index] = pCur->person;
		index++;
	}

	// Simple insertion sort by last name, then first name.
	for (unsigned int i = 1; i < sizeOfUserArray; i++)
	{
		cPerson key = pAllTheUsers[i];
		int j = static_cast<int>(i) - 1;

		while (j >= 0)
		{
			bool shouldSwap = false;

			if (pAllTheUsers[j].last > key.last)
			{
				shouldSwap = true;
			}
			else if (pAllTheUsers[j].last == key.last &&
				pAllTheUsers[j].first > key.first)
			{
				shouldSwap = true;
			}

			if (!shouldSwap)
			{
				break;
			}

			pAllTheUsers[j + 1] = pAllTheUsers[j];
			j--;
		}

		pAllTheUsers[j + 1] = key;
	}

	// If descending requested, reverse the array in-place.
	if (!sortAscending)
	{
		unsigned int start = 0;
		unsigned int end = sizeOfUserArray - 1;
		while (start < end)
		{
			cPerson temp = pAllTheUsers[start];
			pAllTheUsers[start] = pAllTheUsers[end];
			pAllTheUsers[end] = temp;
			start++;
			end--;
		}
	}

	return true;
}

// Sorded ASCENDING by Songify ID
bool cSongify::GetUsersByID(cPerson*& pAllTheUsers, unsigned int& sizeOfUserArray)
{
	pAllTheUsers = NULL;
	sizeOfUserArray = 0;

	if (this->m_numUsers == 0)
	{
		return true;
	}

	sizeOfUserArray = this->m_numUsers;
	pAllTheUsers = new cPerson[sizeOfUserArray];

	unsigned int index = 0;
	for (sUserNode* pCur = this->m_pHeadUser; pCur != NULL; pCur = pCur->pNext)
	{
		pAllTheUsers[index] = pCur->person;
		index++;
	}

	// Insertion sort by Songify unique ID (ascending).
	for (unsigned int i = 1; i < sizeOfUserArray; i++)
	{
		cPerson key = pAllTheUsers[i];
		int j = static_cast<int>(i) - 1;

		while (j >= 0 && pAllTheUsers[j].getSongifyUniqueUserID() > key.getSongifyUniqueUserID())
		{
			pAllTheUsers[j + 1] = pAllTheUsers[j];
			j--;
		}

		pAllTheUsers[j + 1] = key;
	}

	return true;
}

// If an identical song is added, it's ignored.
bool cSongify::AddSong(cSong newSong, std::string& errorString)
{
	// Clear any previous error message.
	errorString = "";

	unsigned int newID = newSong.getUniqueID();

	// Check for duplicate unique ID.
	if (this->m_FindSongByUniqueID(newID) != NULL)
	{
		// Treat duplicates as a successful no-op.
		return true;
	}

	// Ensure capacity in the global song catalogue.
	if (!this->m_EnsureSongCapacity(this->m_numSongs + 1))
	{
		errorString = "AddSong(): memory allocation failure.";
		return false;
	}

	// Create a heap-allocated copy and store the pointer.
	cSong* pNewSong = new cSong(newSong);
	this->m_pSongs[this->m_numSongs] = pNewSong;
	this->m_numSongs++;

	return true;
}

// This finds a match, based on the "unique ID", and overwrites the data. 
bool cSongify::UpdateSong(cSong theSong, std::string& errorString)
{
	// Clear any previous error message.
	errorString = "";

	unsigned int id = theSong.getUniqueID();
	cSong* pExisting = this->m_FindSongByUniqueID(id);
	if (pExisting == NULL)
	{
		errorString = "UpdateSong(): song not found.";
		return false;
	}

	// Overwrite the existing song data.
	*pExisting = theSong;
	return true;
}

// ************************************************************************
// NOTE: Deleting the song deletes it from ALL users that have this song!!!
// ************************************************************************
bool cSongify::DeleteSong(unsigned int UniqueSongID, std::string& errorString)
{
	// Clear any previous error message.
	errorString = "";

	// Locate song in the global catalogue.
	int songIndex = -1;
	for (unsigned int i = 0; i < this->m_numSongs; i++)
	{
		if (this->m_pSongs[i] != NULL &&
			this->m_pSongs[i]->getUniqueID() == UniqueSongID)
		{
			songIndex = static_cast<int>(i);
			break;
		}
	}

	if (songIndex < 0)
	{
		errorString = "DeleteSong(): song not found.";
		return false;
	}

	cSong* pSongToDelete = this->m_pSongs[songIndex];

	// Remove the song pointer from the global array.
	for (unsigned int i = static_cast<unsigned int>(songIndex); i < this->m_numSongs - 1; i++)
	{
		this->m_pSongs[i] = this->m_pSongs[i + 1];
	}
	this->m_numSongs--;

	// Remove this song from all users' libraries.
	this->m_RemoveSongFromAllUsers(pSongToDelete);

	// Finally delete the song object.
	delete pSongToDelete;
	return true;
}

// This associates a particular song with a particular user. 
// (Note: returns true if the song is already there)
bool cSongify::AddSongToUserLibrary(unsigned int songifyUserID, unsigned int UniqueSongID, std::string& errorString)
{
	// Clear any previous error message.
	errorString = "";

	sUserNode* pUser = this->m_FindUserNodeBySongifyID(songifyUserID);
	if (pUser == NULL)
	{
		errorString = "AddSongToUserLibrary(): user not found.";
		return false;
	}

	cSong* pSong = this->m_FindSongByUniqueID(UniqueSongID);
	if (pSong == NULL)
	{
		errorString = "AddSongToUserLibrary(): song not found.";
		return false;
	}

	// Check if the song is already in this user's library.
	for (unsigned int i = 0; i < pUser->librarySize; i++)
	{
		if (pUser->pLibrary[i].pSong == pSong)
		{
			// Already there; consider this a success.
			return true;
		}
	}

	// Ensure there is room for one more entry.
	if (!this->m_EnsureUserLibraryCapacity(pUser, pUser->librarySize + 1))
	{
		errorString = "AddSongToUserLibrary(): memory allocation failure.";
		return false;
	}

	// Append a new library entry.
	sUserSongEntry& entry = pUser->pLibrary[pUser->librarySize];
	entry.pSong = pSong;
	entry.rating = 0;
	entry.numPlays = 0;

	pUser->librarySize++;
	return true;
}

bool cSongify::RemoveSongFromUserLibrary(unsigned int songifyUserID, unsigned int SongifySongID, std::string& errorString)
{
	// Clear any previous error message.
	errorString = "";

	sUserNode* pUser = this->m_FindUserNodeBySongifyID(songifyUserID);
	if (pUser == NULL)
	{
		errorString = "RemoveSongFromUserLibrary(): user not found.";
		return false;
	}

	if (pUser->librarySize == 0)
	{
		return true;
	}

	// Locate the song in the user's library.
	int index = -1;
	for (unsigned int i = 0; i < pUser->librarySize; i++)
	{
		if (pUser->pLibrary[i].pSong != NULL &&
			pUser->pLibrary[i].pSong->getUniqueID() == SongifySongID)
		{
			index = static_cast<int>(i);
			break;
		}
	}

	if (index < 0)
	{
		errorString = "RemoveSongFromUserLibrary(): song not found in user's library.";
		return false;
	}

	// Shift remaining entries down to fill the gap.
	for (unsigned int i = static_cast<unsigned int>(index); i < pUser->librarySize - 1; i++)
	{
		pUser->pLibrary[i] = pUser->pLibrary[i + 1];
	}
	pUser->librarySize--;

	return true;
}

// Note: this is unique TO THE USER, not the entire Songify system.
// i.e. each user has a different rating for each song.
bool cSongify::UpdateRatingOnSong(unsigned int SongifyUserID, unsigned int songUniqueID, unsigned int newRating)
{
	if (newRating > 5)
	{
		// Ratings are defined from 0 to 5.
		return false;
	}

	sUserNode* pUser = this->m_FindUserNodeBySongifyID(SongifyUserID);
	if (pUser == NULL)
	{
		return false;
	}

	for (unsigned int i = 0; i < pUser->librarySize; i++)
	{
		if (pUser->pLibrary[i].pSong != NULL &&
			pUser->pLibrary[i].pSong->getUniqueID() == songUniqueID)
		{
			pUser->pLibrary[i].rating = newRating;
			return true;
		}
	}

	return false;
}

// Returns false if it can't find the song.
// (Can return optional information on errorString)
// 
// ****************************************************************************
// NOTE: This updates the "numberOfTimesPlayed" in THIS USER'S playlist-library
// ****************************************************************************
bool cSongify::GetSongToPlay(unsigned int SongifyUserID, unsigned int songUniqueID, cSong& foundSong, std::string& errorString)
{
	// Clear any previous error message.
	errorString = "";

	sUserNode* pUser = this->m_FindUserNodeBySongifyID(SongifyUserID);
	if (pUser == NULL)
	{
		errorString = "GetSongToPlay(): user not found.";
		return false;
	}

	for (unsigned int i = 0; i < pUser->librarySize; i++)
	{
		sUserSongEntry& entry = pUser->pLibrary[i];
		if (entry.pSong != NULL &&
			entry.pSong->getUniqueID() == songUniqueID)
		{
			// Update per-user play count.
			entry.numPlays++;

			// Return a copy of the song with this user's stats.
			foundSong = *(entry.pSong);
			foundSong.rating = static_cast<int>(entry.rating);
			foundSong.numberOfTimesPlayed = static_cast<int>(entry.numPlays);
			return true;
		}
	}

	errorString = "GetSongToPlay(): song not found in user's library.";
	return false;
}

// Note that the songRating is "returned" by reference. 
// If it can't find a match (i.e. returns false), then the rating isn't valid. 
bool cSongify::GetCurrentSongRating(unsigned int songifyUserID, unsigned int songUniqueID, unsigned int& songRating)
{
	songRating = 0;

	sUserNode* pUser = this->m_FindUserNodeBySongifyID(songifyUserID);
	if (pUser == NULL)
	{
		return false;
	}

	for (unsigned int i = 0; i < pUser->librarySize; i++)
	{
		const sUserSongEntry& entry = pUser->pLibrary[i];
		if (entry.pSong != NULL &&
			entry.pSong->getUniqueID() == songUniqueID)
		{
			songRating = entry.rating;
			return true;
		}
	}

	return false;
}

bool cSongify::GetCurrentSongNumberOfPlays(unsigned int songifyUserID, unsigned int songUniqueID, unsigned int& numberOfPlays)
{
	numberOfPlays = 0;

	sUserNode* pUser = this->m_FindUserNodeBySongifyID(songifyUserID);
	if (pUser == NULL)
	{
		return false;
	}

	for (unsigned int i = 0; i < pUser->librarySize; i++)
	{
		const sUserSongEntry& entry = pUser->pLibrary[i];
		if (entry.pSong != NULL &&
			entry.pSong->getUniqueID() == songUniqueID)
		{
			numberOfPlays = entry.numPlays;
			return true;
		}
	}

	return false;
}

// This isn't tied to a particular user, so doesn't update any stats
// (You can also assume that with duplicates, it will return the 1st one it finds. I don't care which.)
bool cSongify::FindSong(std::string title, std::string artist, cSong& foundSong)
{
	for (unsigned int i = 0; i < this->m_numSongs; i++)
	{
		if (this->m_pSongs[i] != NULL &&
			this->m_pSongs[i]->name == title &&
			this->m_pSongs[i]->artist == artist)
		{
			foundSong = *(this->m_pSongs[i]);
			return true;
		}
	}
	return false;
}

bool cSongify::FindSong(unsigned int uniqueID, cSong& foundSong)
{
	cSong* pSong = this->m_FindSongByUniqueID(uniqueID);
	if (pSong == NULL)
	{
		return false;
	}

	foundSong = *pSong;
	return true;
}


cSongify::sUserNode* cSongify::m_FindUserNodeBySongifyID(unsigned int SongifyUserID) const
{
	for (sUserNode* pCur = this->m_pHeadUser; pCur != NULL; pCur = pCur->pNext)
	{
		if (pCur->person.getSongifyUniqueUserID() == SongifyUserID)
		{
			return pCur;
		}
	}
	return NULL;
}

cSongify::sUserNode* cSongify::m_FindUserNodeBySIN(unsigned int SIN, bool& moreThanOne) const
{
	moreThanOne = false;
	sUserNode* pFound = NULL;

	for (sUserNode* pCur = this->m_pHeadUser; pCur != NULL; pCur = pCur->pNext)
	{
		if (pCur->person.SIN == SIN)
		{
			if (pFound == NULL)
			{
				pFound = pCur;
			}
			else
			{
				// Found more than one user with the same SIN.
				moreThanOne = true;
				return NULL;
			}
		}
	}

	return pFound;
}

cSong* cSongify::m_FindSongByUniqueID(unsigned int uniqueID) const
{
	for (unsigned int i = 0; i < this->m_numSongs; i++)
	{
		if (this->m_pSongs[i] != NULL &&
			this->m_pSongs[i]->getUniqueID() == uniqueID)
		{
			return this->m_pSongs[i];
		}
	}
	return NULL;
}

// Ensure the global song catalogue has at least minCapacity slots. sdsdsdsd
bool cSongify::m_EnsureSongCapacity(unsigned int minCapacity)
{
	if (this->m_songCapacity >= minCapacity)
	{
		return true;
	}

	unsigned int newCapacity = (this->m_songCapacity == 0) ? 10 : this->m_songCapacity * 2;
	while (newCapacity < minCapacity)
	{
		newCapacity *= 2;
	}

	cSong** pNewArray = new cSong * [newCapacity];
	for (unsigned int i = 0; i < newCapacity; i++)
	{
		pNewArray[i] = NULL;
	}

	for (unsigned int i = 0; i < this->m_numSongs; i++)
	{
		pNewArray[i] = this->m_pSongs[i];
	}

	if (this->m_pSongs != NULL)
	{
		delete[] this->m_pSongs;
	}
	this->m_pSongs = pNewArray;
	this->m_songCapacity = newCapacity;

	return true;
}

// Ensure the given user's library has at least minCapacity slots.
bool cSongify::m_EnsureUserLibraryCapacity(sUserNode* pUser, unsigned int minCapacity)
{
	if (pUser == NULL)
	{
		return false;
	}

	if (pUser->libraryCapacity >= minCapacity)
	{
		return true;
	}

	unsigned int newCapacity = (pUser->libraryCapacity == 0) ? 10 : pUser->libraryCapacity * 2;
	while (newCapacity < minCapacity)
	{
		newCapacity *= 2;
	}

	sUserSongEntry* pNewArray = new sUserSongEntry[newCapacity];
	for (unsigned int i = 0; i < newCapacity; i++)
	{
		pNewArray[i].pSong = NULL;
		pNewArray[i].rating = 0;
		pNewArray[i].numPlays = 0;
	}

	for (unsigned int i = 0; i < pUser->librarySize; i++)
	{
		pNewArray[i] = pUser->pLibrary[i];
	}

	if (pUser->pLibrary != NULL)
	{
		delete[] pUser->pLibrary;
	}

	pUser->pLibrary = pNewArray;
	pUser->libraryCapacity = newCapacity;

	return true;
}

// Remove a song from every user's library.
void cSongify::m_RemoveSongFromAllUsers(cSong* pSong)
{
	if (pSong == NULL)
	{
		return;
	}

	for (sUserNode* pCur = this->m_pHeadUser; pCur != NULL; pCur = pCur->pNext)
	{
		unsigned int i = 0;
		while (i < pCur->librarySize)
		{
			if (pCur->pLibrary[i].pSong == pSong)
			{
				// Shift entries down.
				for (unsigned int j = i; j < pCur->librarySize - 1; j++)
				{
					pCur->pLibrary[j] = pCur->pLibrary[j + 1];
				}
				pCur->librarySize--;
				// Do not increment i, as we need to check the new item at this index.
			}
			else
			{
				i++;
			}
		}
	}
}

// Delete all users and their libraries.
void cSongify::m_DeleteAllUsers()
{
	sUserNode* pCur = this->m_pHeadUser;
	while (pCur != NULL)
	{
		sUserNode* pNext = pCur->pNext;

		if (pCur->pLibrary != NULL)
		{
			delete[] pCur->pLibrary;
			pCur->pLibrary = NULL;
		}

		delete pCur;
		pCur = pNext;
	}

	this->m_pHeadUser = NULL;
	this->m_pTailUser = NULL;
	this->m_numUsers = 0;
}

// Delete all songs from the global catalogue.
void cSongify::m_DeleteAllSongs()
{
	if (this->m_pSongs != NULL)
	{
		for (unsigned int i = 0; i < this->m_numSongs; i++)
		{
			if (this->m_pSongs[i] != NULL)
			{
				delete this->m_pSongs[i];
				this->m_pSongs[i] = NULL;
			}
		}

		delete[] this->m_pSongs;
		this->m_pSongs = NULL;
	}

	this->m_numSongs = 0;
	this->m_songCapacity = 0;
}
