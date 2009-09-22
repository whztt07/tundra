#ifndef incl_Comm_User_h
#define incl_Comm_User_h

#include "Foundation.h"
#include "EventDataInterface.h"

#include <QObject>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/Connection>

#include "PresenceStatus.h"
#include "Contact.h"

namespace TpQt4Communication
{
	/**
	 *  Represents an user account in one IM server. There is one User object per Connection object.
	 *
	 *  User object is created when a connection to IM server is made. You can get 
	 *  User object by calling connection.GetUser()
	 *
	 */
	class User : QObject
	{
		Q_OBJECT
		friend class Connection;
	public:
		User(Tp::ConnectionPtr tp_connection);
		std::string GetUserID();
		std::string GetProtocol();
		void SetPresenceStatus(std::string state, std::string message);
		PresenceStatus* GetPresenceStatus();
		ContactVector GetContacts();
	private:

		//! Add new contacts to contact list
		void AddContacts(ContactVector &contacts);

		std::string user_id_;
		std::string protocol_;
		PresenceStatus presence_status_;
		Tp::ConnectionPtr tp_connection_;
		Tp::ContactPtr tp_contact_;
		ContactVector contacts_;
	
	signals:
		void ContactListChangend();

	};
	typedef boost::weak_ptr<User> UserWeakPtr;

} // end of TpQt4Communication: 

#endif // incl_Comm_User_h
