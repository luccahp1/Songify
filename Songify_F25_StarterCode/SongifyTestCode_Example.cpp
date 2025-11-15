#include "cMusicGenerator.h"
#include "cPersonGenerator.h"
#include "cSong.h"
#include "cPerson.h"

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>

#include "cSongify.h"
#include "cLogger.h"

// Michael Feeney, 2025
// mfeeney (at) fanshawec.ca
//

// Assumes you're passing 6 characters
std::string printablePostalCode(char postalCode[/*6*/]);
// Assumes you're passing 11 characters
// NOTE: Stored as NUMBERS *NOT* ASCII values (like the postal code)
std::string printablePhoneNumber(char phoneNumber[/*11*/]);
// Social Insurance Numbers
// https://en.wikipedia.org/wiki/Social_insurance_number or https://www.canada.ca/en/employment-social-development/services/sin.html
std::string printableSIN(unsigned int numSIN);

void Log(std::string theText);

int SongifyTestCode_Example(void)
{

	cLogger* pLog = new cLogger("Bob Student SN:123456");
	pLog->bAddTimeToLogLines = true;



	cPersonGenerator* pPG = new cPersonGenerator();
	cMusicGenerator* pMG = new cMusicGenerator();

	std::string errorString;
	std::string babyNameFile = "Data/Popular Baby Names/names/yob2019.txt";
	std::string surnameFile = "Data/Surnames/names/Names_2010Census.csv";
	std::string streetNameFile = "Data/Street Names (San Francisco)/Street_Names.csv";

	std::cout << "Loading the census data...";
	if ( ! pPG->LoadCensusFiles(babyNameFile, surnameFile, streetNameFile, errorString) )
	{
		std::cout << "\nDidn't load census files because: " << errorString << std::endl;
		delete pPG;
		delete pMG;
		return -1;
	}
	std::cout << "done." << std::endl;


	std::cout << "Loading billboard music files...";
	std::string billboardFileName = "Data/billboard/kcmillersean-billboard-hot-100-1958-2017/data/audiio.csv";
	if ( ! pMG->LoadMusicInformation(billboardFileName, errorString) )
	{
		std::cout << "\nDidn't load music information because: " << errorString << std::endl;
		delete pPG;
		delete pMG;
		return -1;
	}
	std::cout << "done." << std::endl;


	// Generate some people
	std::vector<cPerson> vecPeople;
	for (unsigned int count = 0; count != 10; count++)
	{
		cPerson aPerson = pPG->generateRandomPerson(true);
		vecPeople.push_back(aPerson);
	}

	// Generate some common songs
	std::vector<cSong> vecSharedSongs;
	for ( unsigned int count = 0; count != 50; count++)
	{
		cSong aSong = pMG->getRandomSong();
		vecSharedSongs.push_back(aSong);
	}


	// Here are some suggestions for testing your library. 
	// This should give you an idea how your project will get marked. 
	// Note: This list is NOT exhaustive - the tester might test other "edge cases"


	// Instantiate the student's Songify instance
	cSongify* pSongify = new cSongify();


	// Add user, FindUser(ID), compare

	// Simple sanity test: add one user, then find them by Songify ID.
	{
		cPerson testUser = vecPeople[0];   // take the first generated person
		std::string localError;

		bool addOk = pSongify->AddUser(testUser, localError);
		std::cout << "AddUser returned: " << (addOk ? "true" : "false")
			<< "  (error: " << localError << ")" << std::endl;

		// try to find the same user by their Songify ID
		cPerson foundUser;
		bool findOk = pSongify->FindUserBySongifyID(
			testUser.getSongifyUniqueUserID(), foundUser);

		std::cout << "FindUserBySongifyID returned: "
			<< (findOk ? "true" : "false") << std::endl;

		if (findOk)
		{
			std::cout << "Found: " << foundUser.first << " " << foundUser.last
				<< "  (ID: " << foundUser.getSongifyUniqueUserID() << ")"
				<< std::endl;
		}
	}
	

	// Add SAME user - check return (should be false)


	// Add all users
	// GetUsers(), compare and check order (LAST name, then FIRST then reverse)
	// GetUsersByID(), compare and check order (by SongID)
		// Add all users and test GetUsers / GetUsersByID
	{
		std::cout << "\n=== Testing GetUsers / GetUsersByID ===\n";
		std::string localError;

		// Add all generated people as Songify users.
		for (unsigned int i = 0; i < vecPeople.size(); i++)
		{
			bool added = pSongify->AddUser(vecPeople[i], localError);
			std::cout << "AddUser(" << vecPeople[i].first << " " << vecPeople[i].last
				<< ") returned: " << (added ? "true" : "false")
				<< "  (error: " << localError << ")" << std::endl;
		}

		cPerson* pUsers = NULL;
		unsigned int numUsers = 0;

		// Ascending by LAST, then FIRST
		if (pSongify->GetUsers(pUsers, numUsers, true))
		{
			std::cout << "\nUsers sorted ASC by last, then first:\n";
			for (unsigned int i = 0; i < numUsers; i++)
			{
				std::cout << "  " << pUsers[i].last << ", " << pUsers[i].first
					<< " (ID: " << pUsers[i].getSongifyUniqueUserID() << ")\n";
			}
			delete[] pUsers;
			pUsers = NULL;
		}

		// Descending by LAST, then FIRST
		if (pSongify->GetUsers(pUsers, numUsers, false))
		{
			std::cout << "\nUsers sorted DESC by last, then first:\n";
			for (unsigned int i = 0; i < numUsers; i++)
			{
				std::cout << "  " << pUsers[i].last << ", " << pUsers[i].first
					<< " (ID: " << pUsers[i].getSongifyUniqueUserID() << ")\n";
			}
			delete[] pUsers;
			pUsers = NULL;
		}

		// Ascending by Songify ID
		if (pSongify->GetUsersByID(pUsers, numUsers))
		{
			std::cout << "\nUsers sorted ASC by Songify ID:\n";
			for (unsigned int i = 0; i < numUsers; i++)
			{
				std::cout << "  ID: " << pUsers[i].getSongifyUniqueUserID()
					<< "  " << pUsers[i].last << ", " << pUsers[i].first << "\n";
			}
			delete[] pUsers;
			pUsers = NULL;
		}
	}

	// Test ratings, plays, and sorted library
	{
		std::cout << "\n=== Testing ratings, plays, and sorted library ===\n";
		std::string localError;

		cPerson firstUser = vecPeople[0];
		unsigned int userID = firstUser.getSongifyUniqueUserID();

		// Use a few songs from the shared pool
		cSong s0 = vecSharedSongs[0];
		cSong s1 = vecSharedSongs[1];
		cSong s2 = vecSharedSongs[2];

		// Make sure they exist in the global catalogue
		pSongify->AddSong(s0, localError);
		pSongify->AddSong(s1, localError);
		pSongify->AddSong(s2, localError);

		// Ensure they are in this user's library
		pSongify->AddSongToUserLibrary(userID, s0.getUniqueID(), localError);
		pSongify->AddSongToUserLibrary(userID, s1.getUniqueID(), localError);
		pSongify->AddSongToUserLibrary(userID, s2.getUniqueID(), localError);

		// Set ratings: s0=3, s1=5, s2=1
		pSongify->UpdateRatingOnSong(userID, s0.getUniqueID(), 3);
		pSongify->UpdateRatingOnSong(userID, s1.getUniqueID(), 5);
		pSongify->UpdateRatingOnSong(userID, s2.getUniqueID(), 1);

		// Play counts: s0 once, s1 twice, s2 three times
		cSong tempSong;
		pSongify->GetSongToPlay(userID, s0.getUniqueID(), tempSong, localError); // 1 play

		pSongify->GetSongToPlay(userID, s1.getUniqueID(), tempSong, localError); // 1
		pSongify->GetSongToPlay(userID, s1.getUniqueID(), tempSong, localError); // 2

		pSongify->GetSongToPlay(userID, s2.getUniqueID(), tempSong, localError); // 1
		pSongify->GetSongToPlay(userID, s2.getUniqueID(), tempSong, localError); // 2
		pSongify->GetSongToPlay(userID, s2.getUniqueID(), tempSong, localError); // 3

		// Show current ratings and plays via getters
		unsigned int rating = 0;
		unsigned int plays = 0;

		std::cout << "Ratings and plays for user " << userID << ":\n";

		if (pSongify->GetCurrentSongRating(userID, s0.getUniqueID(), rating) &&
			pSongify->GetCurrentSongNumberOfPlays(userID, s0.getUniqueID(), plays))
		{
			std::cout << "  " << s0.name << " rating=" << rating << " plays=" << plays << "\n";
		}

		if (pSongify->GetCurrentSongRating(userID, s1.getUniqueID(), rating) &&
			pSongify->GetCurrentSongNumberOfPlays(userID, s1.getUniqueID(), plays))
		{
			std::cout << "  " << s1.name << " rating=" << rating << " plays=" << plays << "\n";
		}

		if (pSongify->GetCurrentSongRating(userID, s2.getUniqueID(), rating) &&
			pSongify->GetCurrentSongNumberOfPlays(userID, s2.getUniqueID(), plays))
		{
			std::cout << "  " << s2.name << " rating=" << rating << " plays=" << plays << "\n";
		}

		// Sorted by rating (high -> low)
		cSong* pLib = NULL;
		unsigned int libSize = 0;

		if (pSongify->GetUsersSongLibraryAscendingBySongRating(userID, pLib, libSize))
		{
			std::cout << "\nLibrary sorted by rating (high->low):\n";
			for (unsigned int i = 0; i < libSize; i++)
			{
				std::cout << "  " << pLib[i].name
					<< " rating=" << pLib[i].rating
					<< " plays=" << pLib[i].numberOfTimesPlayed << "\n";
			}
			delete[] pLib;
			pLib = NULL;
		}

		// Sorted by plays (high -> low)
		if (pSongify->GetUsersSongLibraryAscendingByNumberOfPlays(userID, pLib, libSize))
		{
			std::cout << "\nLibrary sorted by plays (high->low):\n";
			for (unsigned int i = 0; i < libSize; i++)
			{
				std::cout << "  " << pLib[i].name
					<< " rating=" << pLib[i].rating
					<< " plays=" << pLib[i].numberOfTimesPlayed << "\n";
			}
			delete[] pLib;
			pLib = NULL;
		}
	}


	// Test DeleteSong removing from all user libraries
	{
		std::cout << "\n=== Testing DeleteSong removes song from all user libraries ===\n";
		std::string localError;

		cSong shared = vecSharedSongs[3];
		pSongify->AddSong(shared, localError);

		cPerson userA = vecPeople[0];
		cPerson userB = vecPeople[1];
		unsigned int idA = userA.getSongifyUniqueUserID();
		unsigned int idB = userB.getSongifyUniqueUserID();

		pSongify->AddSongToUserLibrary(idA, shared.getUniqueID(), localError);
		pSongify->AddSongToUserLibrary(idB, shared.getUniqueID(), localError);

		cSong* pLib = NULL;
		unsigned int libSize = 0;

		// Libraries BEFORE delete
		if (pSongify->GetUsersSongLibrary(idA, pLib, libSize))
		{
			std::cout << "User A library BEFORE DeleteSong (size=" << libSize << "):\n";
			for (unsigned int i = 0; i < libSize; i++)
			{
				std::cout << "  " << pLib[i].name << "\n";
			}
			delete[] pLib;
			pLib = NULL;
		}

		if (pSongify->GetUsersSongLibrary(idB, pLib, libSize))
		{
			std::cout << "User B library BEFORE DeleteSong (size=" << libSize << "):\n";
			for (unsigned int i = 0; i < libSize; i++)
			{
				std::cout << "  " << pLib[i].name << "\n";
			}
			delete[] pLib;
			pLib = NULL;
		}

		// Delete the song globally
		bool delOk = pSongify->DeleteSong(shared.getUniqueID(), localError);
		std::cout << "DeleteSong returned: " << (delOk ? "true" : "false")
			<< " (error: " << localError << ")\n";

		// Libraries AFTER delete
		if (pSongify->GetUsersSongLibrary(idA, pLib, libSize))
		{
			std::cout << "User A library AFTER DeleteSong (size=" << libSize << "):\n";
			for (unsigned int i = 0; i < libSize; i++)
			{
				std::cout << "  " << pLib[i].name << "\n";
			}
			delete[] pLib;
			pLib = NULL;
		}

		if (pSongify->GetUsersSongLibrary(idB, pLib, libSize))
		{
			std::cout << "User B library AFTER DeleteSong (size=" << libSize << "):\n";
			for (unsigned int i = 0; i < libSize; i++)
			{
				std::cout << "  " << pLib[i].name << "\n";
			}
			delete[] pLib;
			pLib = NULL;
		}
	}

	// Test DeleteUser
	{
		std::cout << "\n=== Testing DeleteUser ===\n";
		std::string localError;

		cPerson userToDelete = vecPeople[2];
		unsigned int idToDelete = userToDelete.getSongifyUniqueUserID();

		cPerson* pUsers = NULL;
		unsigned int numUsers = 0;

		// Users BEFORE delete
		if (pSongify->GetUsersByID(pUsers, numUsers))
		{
			std::cout << "Users before DeleteUser: " << numUsers << "\n";
			delete[] pUsers;
			pUsers = NULL;
		}

		bool delOk = pSongify->DeleteUser(idToDelete, localError);
		std::cout << "DeleteUser returned: " << (delOk ? "true" : "false")
			<< " (error: " << localError << ")\n";

		// Users AFTER delete
		if (pSongify->GetUsersByID(pUsers, numUsers))
		{
			std::cout << "Users after DeleteUser: " << numUsers << "\n";
			delete[] pUsers;
			pUsers = NULL;
		}

		// Try to find the deleted user by Songify ID
		cPerson found;
		bool foundOk = pSongify->FindUserBySongifyID(idToDelete, found);
		std::cout << "FindUserBySongifyID for deleted user returned: "
			<< (foundOk ? "true" : "false") << "\n";
	}

	// AddSong(), then FindSong(ID) and FindSong(title & artist)
	// AddSong() DUPLICATE, check return
	{
		std::string localError;

		// Use the first shared song from the generated list
		cSong song = vecSharedSongs[0];

		bool addSongOk = pSongify->AddSong(song, localError);
		std::cout << "AddSong returned: " << (addSongOk ? "true" : "false")
			<< "  (error: " << localError << ")" << std::endl;

		// Now try to add the same song again (should be treated as duplicate)
		bool addSongDuplicateOk = pSongify->AddSong(song, localError);
		std::cout << "AddSong (duplicate) returned: "
			<< (addSongDuplicateOk ? "true" : "false")
			<< "  (error: " << localError << ")" << std::endl;

		// Test FindSong by ID
		cSong foundByID;
		bool foundID = pSongify->FindSong(song.getUniqueID(), foundByID);
		std::cout << "FindSong(ID) returned: " << (foundID ? "true" : "false")
			<< "  name: " << foundByID.name << std::endl;

		// Test FindSong by title & artist
		cSong foundByTA;
		bool foundTA = pSongify->FindSong(song.name, song.artist, foundByTA);
		std::cout << "FindSong(title, artist) returned: "
			<< (foundTA ? "true" : "false")
			<< "  name: " << foundByTA.name << std::endl;

		// Now associate that song with the user we added earlier
		cPerson firstUser = vecPeople[0];
		bool addToLibOk = pSongify->AddSongToUserLibrary(
			firstUser.getSongifyUniqueUserID(),
			song.getUniqueID(),
			localError);

		std::cout << "AddSongToUserLibrary returned: "
			<< (addToLibOk ? "true" : "false")
			<< "  (error: " << localError << ")" << std::endl;

		// Play the song once for that user
		cSong songToPlay;
		bool playOk = pSongify->GetSongToPlay(
			firstUser.getSongifyUniqueUserID(),
			song.getUniqueID(),
			songToPlay,
			localError);

		std::cout << "GetSongToPlay returned: " << (playOk ? "true" : "false")
			<< "  (error: " << localError << ")" << std::endl;

		if (playOk)
		{
			std::cout << "After playing, numberOfTimesPlayed = "
				<< songToPlay.numberOfTimesPlayed << std::endl;
		}
	}

	// Test FindUserBySIN and UpdateUser (valid + invalid)
	{
		std::cout << "\n=== Testing FindUserBySIN and UpdateUser ===\n";
		std::string localError;

		// Pick a user we know is in Songify (first person from vecPeople)
		cPerson original = vecPeople[0];
		unsigned int originalID = original.getSongifyUniqueUserID();
		unsigned int originalSIN = original.SIN;

		// Ensure they're added if they're already there, this will just fail gracefully
		pSongify->AddUser(original, localError);

		// FindUserBySIN
		cPerson foundBySIN;
		bool foundSIN = pSongify->FindUserBySIN(originalSIN, foundBySIN);
		std::cout << "FindUserBySIN returned: " << (foundSIN ? "true" : "false") << "\n";
		if (foundSIN)
		{
			std::cout << "  Found: " << foundBySIN.first << " " << foundBySIN.last
				<< " (ID=" << foundBySIN.getSongifyUniqueUserID()
				<< ", SIN=" << foundBySIN.SIN << ")\n";
		}

		// Valid UpdateUser:
		//    change the last name and update, then re-find by ID.
		if (foundSIN)
		{
			foundBySIN.last = "UPDATED_LAST_NAME";
			bool updateOk = pSongify->UpdateUser(foundBySIN, localError);
			std::cout << "UpdateUser (valid) returned: "
				<< (updateOk ? "true" : "false")
				<< " (error: " << localError << ")\n";

			cPerson afterUpdate;
			bool foundAfter = pSongify->FindUserBySongifyID(originalID, afterUpdate);
			std::cout << "FindUserBySongifyID after valid update: "
				<< (foundAfter ? "true" : "false") << "\n";
			if (foundAfter)
			{
				std::cout << "  After update last name: " << afterUpdate.last << "\n";
			}
		}

		// Invalid UpdateUser:
		//    mess up the SIN so the combination (ID + SIN) doesn't match any stored user.
		cPerson bogus = original;
		bogus.SIN = originalSIN + 1; // unlikely to match the stored SIN

		bool updateBad = pSongify->UpdateUser(bogus, localError);
		std::cout << "UpdateUser (invalid ID+SIN combo) returned: "
			<< (updateBad ? "true" : "false")
			<< " (error: " << localError << ")\n";
	}

	// Test duplicate AddSongToUserLibrary and RemoveSongFromUserLibrary
	{
		std::cout << "\n=== Testing AddSongToUserLibrary duplicate and RemoveSongFromUserLibrary ===\n";
		std::string localError;

		// Choose a user and a song
		cPerson user = vecPeople[1];
		unsigned int userID = user.getSongifyUniqueUserID();
		cSong song = vecSharedSongs[4];

		// Make sure user and song exist
		pSongify->AddUser(user, localError);
		pSongify->AddSong(song, localError);

		// Add song to user's library twice
		bool add1 = pSongify->AddSongToUserLibrary(userID, song.getUniqueID(), localError);
		bool add2 = pSongify->AddSongToUserLibrary(userID, song.getUniqueID(), localError);

		std::cout << "First AddSongToUserLibrary returned: "
			<< (add1 ? "true" : "false")
			<< " (error: " << localError << ")\n";
		std::cout << "Second AddSongToUserLibrary (duplicate) returned: "
			<< (add2 ? "true" : "false")
			<< " (error: " << localError << ")\n";

		// Check library size – should be 1, not 2
		cSong* pLib = NULL;
		unsigned int libSize = 0;
		if (pSongify->GetUsersSongLibrary(userID, pLib, libSize))
		{
			std::cout << "Library size after duplicate adds: " << libSize << "\n";
			for (unsigned int i = 0; i < libSize; i++)
			{
				std::cout << "  " << pLib[i].name << "\n";
			}
			delete[] pLib;
			pLib = NULL;
		}

		// Now remove teh song
		bool removeOk = pSongify->RemoveSongFromUserLibrary(userID, song.getUniqueID(), localError);
		std::cout << "RemoveSongFromUserLibrary returned: "
			<< (removeOk ? "true" : "false")
			<< " (error: " << localError << ")\n";

		// Library should now be empty (or at least not contain that song)
		if (pSongify->GetUsersSongLibrary(userID, pLib, libSize))
		{
			std::cout << "Library size after remove: " << libSize << "\n";
			for (unsigned int i = 0; i < libSize; i++)
			{
				std::cout << "  " << pLib[i].name << "\n";
			}
			delete[] pLib;
			pLib = NULL;
		}
	}


	// Test some invalid inputs (DeleteUser invalid, GetSongToPlay invalid, FindSong invalid)
	{
		std::cout << "\n=== Testing invalid input cases ===\n";
		std::string localError;

		// 1) DeleteUser with an ID that almost certainly doesn't exist
		unsigned int bogusUserID = 999999999;
		bool delBad = pSongify->DeleteUser(bogusUserID, localError);
		std::cout << "DeleteUser(invalid ID) returned: "
			<< (delBad ? "true" : "false")
			<< " (error: " << localError << ")\n";

		// 2) GetSongToPlay for a user that doesn't exist
		cSong tempSong;
		bool playBadUser = pSongify->GetSongToPlay(bogusUserID, 123456u, tempSong, localError);
		std::cout << "GetSongToPlay(invalid user) returned: "
			<< (playBadUser ? "true" : "false")
			<< " (error: " << localError << ")\n";

		// 3) GetSongToPlay for a valid user but bogus song ID
		if (!vecPeople.empty())
		{
			unsigned int realUserID = vecPeople[0].getSongifyUniqueUserID();
			bool playBadSong = pSongify->GetSongToPlay(realUserID, 99999999u, tempSong, localError);
			std::cout << "GetSongToPlay(invalid song for valid user) returned: "
				<< (playBadSong ? "true" : "false")
				<< " (error: " << localError << ")\n";
		}

		// 4) FindSong by an ID that should not exist
		cSong notFoundSong;
		bool foundBad = pSongify->FindSong(123456789u, notFoundSong);
		std::cout << "FindSong(invalid ID) returned: "
			<< (foundBad ? "true" : "false") << "\n";
	}

	delete pLog;

	delete pSongify;


	delete pPG;
	delete pMG;

	return 0;
}

void Log(std::string theText)
{
}