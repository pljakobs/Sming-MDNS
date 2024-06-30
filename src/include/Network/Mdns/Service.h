/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/SmingHub/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 *
 * Service.h
 *
 ****/

#pragma once

#include "Request.h"
#include <Data/LinkedObjectList.h>
#include <Data/CStringArray.h>
#include <WString.h>

namespace mDNS
{
/**
 * @brief ".local"
 */
DECLARE_FSTR(fstrDotLocal)

class Responder;

/**
 * @brief Describes a basic service
 *
 * The default methods translate to a DNS-SD name of "Sming._http._tcp.local".
 * See :cpp:class:`mDNS::Name` for a description of how names are defined.
 */
class Service : public LinkedObjectTemplate<Service>
{
public:
	using List = LinkedObjectListTemplate<Service>;

	enum class Protocol {
		Udp,
		Tcp,
	};

	/**
	 * @brief User-friendly Instance name
	 */
	virtual String getInstance()
	{
		return F("Sming");
	}

	/**
	 * @brief Identifies what the service does
	 */
	virtual String getName()
	{
		return F("http");
	}

	/**
	 * @brief Which protocol the service uses
	 */
	virtual Protocol getProtocol()
	{
		return Protocol::Tcp;
	}

	/**
	 * @brief Which port to access service on
	 */
	virtual uint16_t getPort()
	{
		return 80;
	};

	/**
	 * @brief Override to obtain txt items
	 * @param txt Resource to add text items, e.g. name=value pairs
	 *
	 * Called whenever a TXT reply is created.
	 */
	virtual void addText([[maybe_unused]] mDNS::Resource::TXT& txt)
	{
	}

	/**
	 * @brief Get advertised service instance name
	 * 
	 * e.g. "Sming._http._tcp.local"
	 */
	String getInstanceName();

	/**
	 * @brief Get advertised service name (without the instance)
	 * 
	 * e.g. "_http._tcp.local"
	 */
	String getServiceName();
	void getServiceName(String& s);

	// Cache certain data to improve performance
	struct Host {
		String name;
		String nameWithDomain;
	};

	bool handleQuestion(Question& question, const Host& host, Request& reply);
};

} // namespace mDNS

String toString(mDNS::Service::Protocol protocol);
