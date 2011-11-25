/*
 * SpaceWireIF.hh
 *
 *  Created on: Aug 3, 2011
 *      Author: yuasa
 */

#ifndef SPACEWIREIF_HH_
#define SPACEWIREIF_HH_

#include <CxxUtilities/CommonHeader.hh>
#include <CxxUtilities/Exception.hh>
#include "SpaceWireEOPMarker.hh"

class SpaceWireIFException: public CxxUtilities::Exception {
public:
	enum {
		OpeningConnectionFailed, Disconnected, Timeout, EEP, ReceiveBufferTooSmall, FunctionNotImplemented
	};
public:
	SpaceWireIFException(uint32_t status) :
		CxxUtilities::Exception(status) {
	}
};

/** An abstract class which includes a method invoked when
 * Timecode is received.
 */
class TimecodeScynchronizedAction {
public:
	virtual void doAction(unsigned char timeOut)=0;
};

/** An abstract class for encapsulation of a SpaceWire interface.
 *  This class provides virtual methods for opening/closing the interface
 *  and sending/receiving a packet via the interface.
 *  Physical or Virtual SpaceWire interface class should be created
 *  by inheriting this class.
 */
class SpaceWireIF {
private:
	std::vector<TimecodeScynchronizedAction*> timecodeSynchronizedActions;
	bool isTerminatedWithEEP_;
	bool isTerminatedWithEOP_;

protected:
	bool eepShouldBeReportedAsAnException_;//default false (no exception)

public:
	enum {
		EOP = 0x00, EEP = 0x01, Undefined = 0xffff
	} eopType;

public:
	SpaceWireIF() {
		isTerminatedWithEEP_ = false;
		isTerminatedWithEOP_ = false;
		eepShouldBeReportedAsAnException_ = false;
	}

public:
	virtual void open() throw (SpaceWireIFException) =0;

	virtual void close() throw (SpaceWireIFException) =0;

public:
	virtual void
	send(uint8_t* data, uint32_t length, uint32_t eopType = SpaceWireEOPMarker::EOP) throw (SpaceWireIFException) =0;

	virtual void send(std::vector<uint8_t>& data, uint32_t eopType = SpaceWireEOPMarker::EOP)
			throw (SpaceWireIFException) {
		if (data.size() == 0) {
			return;
		} else {
			send(&(data[0]), data.size(), eopType);
		}
	}

	virtual void send(std::vector<uint8_t>* data, uint32_t eopType = SpaceWireEOPMarker::EOP)
			throw (SpaceWireIFException) {
		if (data->size() == 0) {
			return;
		} else {
			send(&(data->at(0)), data->size(), eopType);
		}
	}

public:
	virtual void receive(uint8_t* buffer, uint8_t& eopType, uint32_t maxLength, size_t& length)
			throw (SpaceWireIFException) {
		std::vector<uint8_t>* packet = this->receive();
		size_t packetSize = packet->size();
		if (packetSize == 0) {
			length = 0;
		} else {
			length = packetSize;
			if (packetSize <= maxLength) {
				memcpy(buffer, &(packet->at(0)), packetSize);
			} else {
				memcpy(buffer, &(packet->at(0)), maxLength);
				delete packet;
				throw SpaceWireIFException(SpaceWireIFException::ReceiveBufferTooSmall);
			}
		}
		delete packet;
	}

	virtual std::vector<uint8_t>* receive() throw (SpaceWireIFException) {
		std::vector<uint8_t>* buffer = new std::vector<uint8_t>();
		this->receive(buffer);
		return buffer;
	}

	virtual void receive(std::vector<uint8_t>* buffer) throw (SpaceWireIFException) =0;

public:
	virtual void emitTimecode(uint8_t timeIn, uint8_t controlFlagIn = 0x00) throw (SpaceWireIFException) =0;

public:
	virtual void setTxLinkRate(uint32_t linkRateType) throw (SpaceWireIFException) =0;

	virtual uint32_t getTxLinkRateType() throw (SpaceWireIFException) =0;

private:
	uint32_t txLinkRateType;
	enum {
		LinkRate200MHz = 200000,
		LinkRate125MHz = 125000,
		LinkRate100MHz = 100000,
		LinkRate50MHz = 50000,
		LinkRate25MHz = 25000,
		LinkRate12_5MHz = 12500,
		LinkRate10MHz = 10000,
		LinkRate5MHz = 5000,
		LinkRate2MHz = 2000
	};

public:
	virtual void setTimeoutDuration(double microsecond) throw (SpaceWireIFException) =0;

	double getTimeoutDurationInMicroSec() {
		return timeoutDurationInMicroSec;
	}

protected:
	//if timeoutDurationInMicroSec==0, timeout is disabled.
	double timeoutDurationInMicroSec; //us

public:
	/** A method sets (adds) an action against getting Timecode.
	 */
	void addTimecodeAction(TimecodeScynchronizedAction* action) {
		timecodeSynchronizedActions.push_back(action);
	}

	void deleteTimecodeAction(TimecodeScynchronizedAction* action) {
		std::vector<TimecodeScynchronizedAction*>::iterator it = std::find(timecodeSynchronizedActions.begin(),
				timecodeSynchronizedActions.end(), action);
		if (it != timecodeSynchronizedActions.end()) {
			timecodeSynchronizedActions.erase(it);
		}
	}

	void clearTimecodeSynchronizedActions() {
		timecodeSynchronizedActions.clear();
	}

	void invokeTimecodeSynchronizedActions(uint8_t tickOutValue) {
		for (size_t i = 0; i < timecodeSynchronizedActions.size(); i++) {
			timecodeSynchronizedActions[i]->doAction(tickOutValue);
		}
	}

public:
	bool isTerminatedWithEEP() {
		return isTerminatedWithEEP_;
	}

	bool isTerminatedWithEOP() {
		return isTerminatedWithEOP_;
	}

	void setReceivedPacketEOPMarkerType(int eopType) {
		if (eopType == SpaceWireIF::EEP) {
			isTerminatedWithEEP_ = true;
			isTerminatedWithEEP_ = false;
		} else if (eopType == SpaceWireIF::EOP) {
			isTerminatedWithEEP_ = false;
			isTerminatedWithEEP_ = true;
		} else {
			isTerminatedWithEEP_ = false;
			isTerminatedWithEEP_ = false;
		}
	}

	int getReceivedPacketEOPMarkerType() {
		if (isTerminatedWithEEP_ == false && isTerminatedWithEEP_ == true) {
			return EOP;
		} else if (isTerminatedWithEEP_ == true && isTerminatedWithEEP_ == false) {
			return EEP;
		} else {
			return Undefined;
		}
	}

	void eepShouldBeReportedAsAnException() {
		eepShouldBeReportedAsAnException_ = true;
	}

	void eepShouldNotBeReportedAsAnException() {
		eepShouldBeReportedAsAnException_ = false;
	}

};
#endif /* SPACEWIREIF_HH_ */
