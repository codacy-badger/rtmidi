/********************* -*- C++ -*- ****************************************/
/*! \class RtMidi
    \brief An abstract base class for realtime MIDI input/output.

    This class implements some common functionality for the realtime
    MIDI input/output subclasses RtMidiIn and RtMidiOut.

    RtMidi WWW site: http://music.mcgill.ca/~gary/rtmidi/

    RtMidi: realtime MIDI i/o C++ classes
    Copyright (c) 2003-2014 Gary P. Scavone

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation files
    (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software,
    and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    Any person wishing to distribute modifications to the Software is
    asked to send the modifications to the original developer so that
    they can be incorporated into the canonical version.  This is,
    however, not a binding provision of this license.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/**********************************************************************/

/*!
  \file RtMidi.h
 */

#ifndef RTMIDI_H
#define RTMIDI_H

#define RTMIDI_VERSION "2.1.0"

#include <exception>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <memory>

/************************************************************************/
/*! \class RtMidiError
    \brief Exception handling class for RtMidi.

    The RtMidiError class is quite simple but it does allow errors to be
    "caught" by RtMidiError::Type. See the RtMidi documentation to know
    which methods can throw an RtMidiError.
*/
/************************************************************************/

class RtMidiError : public std::exception
{
 public:
  //! Defined RtMidiError types.
  enum Type {
    WARNING,           /*!< A non-critical error. */
    DEBUG_WARNING,     /*!< A non-critical error which might be useful for debugging. */
    UNSPECIFIED,       /*!< The default, unspecified error type. */
    NO_DEVICES_FOUND,  /*!< No devices found on system. */
    INVALID_DEVICE,    /*!< An invalid device ID was specified. */
    MEMORY_ERROR,      /*!< An error occured during memory allocation. */
    INVALID_PARAMETER, /*!< An invalid parameter was specified to a function. */
    INVALID_USE,       /*!< The function was called incorrectly. */
    DRIVER_ERROR,      /*!< A system driver error occured. */
    SYSTEM_ERROR,      /*!< A system error occured. */
    THREAD_ERROR       /*!< A thread error occured. */
  };

  //! The constructor.
  RtMidiError( const std::string& message, Type type = RtMidiError::UNSPECIFIED ) throw() : message_(message), type_(type) {}

  //! The destructor.
  virtual ~RtMidiError( void ) throw() {}

  //! Prints thrown error message to stderr.
  virtual void printMessage( void ) const throw() { std::cerr << '\n' << message_ << "\n\n"; }

  //! Returns the thrown error message type.
  virtual const Type& getType(void) const throw() { return type_; }

  //! Returns the thrown error message string.
  virtual const std::string& getMessage(void) const throw() { return message_; }

  //! Returns the thrown error message as a c-style string.
  virtual const char* what( void ) const throw() { return message_.c_str(); }

 protected:
  std::string message_;
  Type type_;
};

//! RtMidi error callback function prototype.
/*!
    \param type Type of error.
    \param errorText Error description.

    Note that class behaviour is undefined after a critical error (not
    a warning) is reported.
 */
typedef void (*RtMidiErrorCallback)( RtMidiError::Type type, const std::string &errorText );

class CommonMidiApi {
public:
	//! A port descriptor type.
	/*! This will be used with scoped pointers to store system dependent
	 *  data for identifying the port.
	 */
	struct PortDescriptor {
		//! Flags for formatting a string description of the port.
		/*! These flags just mark the requirements that the string
		  should fulfil. An API may return the same string for
		  different requirements e.g. the same short and long
		  name. */
		enum NamingType {
			SHORT_NAME =0,  /*!< A short human readable name
					  depending on the API
					  e.g. “Ensoniq AudioPCI” */
			LONG_NAME,      /*!< A complete human readable
					  name depending on the API
					  e.g. "Ensoniq AudioPCI: ES1371" */
			SESSION_PATH,   /*!< A unique description that can
					  be used to identify the port
					  during runtime. It may be a
					  cryptic string. */
			STORAGE_PATH,   /*!< A unique description that is
					  optimised for storage in
					  configuration files. This is a
					  more textual representation that
					  is more robust to small changes in
					  the surrounding environment than
					  \ref SESSION_PATH */
			UNIQUE_NAME = 0x10, /*!< Make all names uniqe. This
					      is usually done by adding
					      numbers to the end of the
					      string */
			INCLUDE_API = 0x20 /*!< Add a string describing the
					     API at the beginning of the
					     string. */
		};

		//! Flags describing the capabilities of a given port.
		enum PortCapabilities {
			INPUT    = 1,  /*!< Ports that can be read from. */
			OUTPUT   = 2,  /*!< Ports that can be written to. */
			INOUTPUT = 3   /*!< Ports that allow reading and writing (INPUT | OUTPUT) */
		};


#if __cplusplus < 201103L
		class Pointer {
		protected:
			struct countPointer {
				int count;
				PortDescriptor * descriptor;
			};
		public:
			Pointer(PortDescriptor * p):ptr(new countPointer) {
				ptr->count = 1;
				ptr->descriptor = p;
			}
			Pointer(const Pointer & other):
				ptr(other.ptr) {
				ptr->count++;
			}

			~Pointer() {
				if (!ptr) return;
				if (!ptr->descriptor) {
					delete ptr;
					return;
				}
				if (!(--ptr->count)) {
					delete ptr->descriptor;
					delete ptr;
				}
			}

			PortDescriptor * operator -> () {
				if (!ptr) return 0;
				// this should throw an exception

				return ptr->descriptor;
			}
		protected:
			countPointer * ptr;
		};
#else
		typedef shared_ptr<PortDescriptor> Pointer;
#endif


		//! Default constructor.
		/*!
		 * Derived classes should have a constructor.
		 */
		PortDescriptor() {};

		//! A virtual destructor
		/*! As we might have to destruct the object from the application code
		 *  each port id must have a virtual destructor.
		 */
		virtual ~PortDescriptor() = 0;

		//! Get the MIDI api for the current port.
		/*! This is the only information RtMidi needs to know: Which
		 *  API should handle this object.
		 *
		 * \return API that can handle this object.
		 */
		virtual CommonMidiApi * getAPI() = 0;

		//! Return the port name
		/*!
		 * \param flags A description of the requirements of the returned name.
		 * \return A name that is formatted according to \ref flags.
		 * \sa NamingTypes
		 */
		virtual std::string getName(int flags = SHORT_NAME | UNIQUE_NAME) = 0;

		//! Get capabilities
		/*! \return a capabilities flag describing the capabilities of the port.
		 *  \sa PortCapabilities
		 */
		virtual int getCapabilities();
	};

	//! A list of port descriptors.
	/*! Port descriptors are stored as shared pointers. This avoids
	  unnecessary duplication of the data structure and handles automatic
	  deletion if all references have been removed. */
	typedef std::list<PortDescriptor::Pointer > PortList;

	//! Pure virtal function to create a virtual port, with optional name.
	/*!
	  This function creates a virtual MIDI port to which other
	  software applications can connect. This type of functionality
	  is currently only supported by the Macintosh OS-X, any JACK,
	  and Linux ALSA APIs (the function returns an error for the other APIs).

	  \param portName An optional name for the applicaction port that is
	  used to connect to portId can be specified.
	*/
	virtual void openVirtualPort( const std::string portName = std::string( "RtMidi" ) ) = 0;

	//! Pure virtual function to open a MIDI connection given by enumeration number.
	/*! \param portNumber An optional port number greater than 0
	  can be specified.  Otherwise, the default or first port
	  found is opened.

	  \param portName An optional name for the applicaction port
	  that will be generated to connect to portId can be
	  specified.
	*/
	virtual void openPort( unsigned int portNumber = 0, const std::string portName = std::string( "RtMidi" ) ) = 0;

	//! Pure virtual function to open a MIDI connection given by a port descriptor.
	/*!
	  \param port     A port descriptor of the port must be specified.
	  \param portName An optional name for the applicaction port that is used to connect to portId can be specified.
	*/
	virtual void openPort( const PortDescriptor & port, const std::string portName = std::string( "RtMidi" ) ) = 0;

	//! Pure virtual function to return a port descirptor if the port is open
	virtual PortDescriptor::Pointer getDescriptor() = 0;

	//! Pure virtual function to return a list of all available ports of the current API.
	/*!
	  \param capabilities an optional parameter that describes which
	  device types are returned.

	  \return This function returns a list of port descriptors.

	  \note An input API may but need not necessarily report
	  output devices which cannot be used as input if
	  \ref PortDescriptor::OUTPUT is passed as \ref capabilities parameter.
	*/
	virtual CommonMidiApi::PortList getPortList(int capabilities = PortDescriptor:: INOUTPUT) = 0;

	//! Pure virtual to return the number of available MIDI ports of the current API.
	/*!
	  \return This function returns the number of MIDI ports of
	  the selected API.

	  \note Only ports are counted that can be used with the
	  current API so an input API does ignore all output devices
	  and vice versa.

	  \sa getPortName
	*/
	virtual unsigned int getPortCount() = 0;

	//! Pure virtual function to return a string identifier for the specified MIDI port number.
	/*!
	  \param portNumber Number of the device to be referred to.
	  \return The name of the port with the given Id is returned.
	  \retval An empty string is returned if an invalid port specifier is provided.

	  \note Only ports are counted that can be used with the
	  current API so an input API does ignore all output devices
	  and vice versa.

	  \sa getPortCount()
	*/
	std::string getPortName( unsigned int portNumber = 0 );

	//! Pure virtual function to close an open MIDI connection (if one exists).
	virtual void closePort( void ) = 0;

	// ! A basic error reporting function for RtMidi classes.
	//  static void error( RtMidiError::Type type, std::string &errorString );

	//! Pure virtual function to return whether a port is open or not.
	/*! \retval true if a port is open and
	   \retval false if the port is not open (e.g. not opend or closed).
	*/
	virtual bool isPortOpen( void ) const = 0;

	//! Pure virtual function to set an error callback function to be invoked when an error has occured.
	/*!
	  The callback function will be called whenever an error has occured. It is best
	  to set the error callback function before opening a port.
	*/
	virtual void setErrorCallback( RtMidiErrorCallback errorCallback = NULL ) = 0;
};

/**********************************************************************/
/*! \class RtMidi
    \brief A global class that implements basic backend API handling.

    This class enhances \ref CommonMidiApi by some functionality to handle
    backend API objects. It serves as base class for the public RtMidi
    API.

    by Gary P. Scavone, 2003-2014.
*/
/**********************************************************************/
class MidiApi;
class RtMidi : public CommonMidiApi
{
 public:

  //! MIDI API specifier arguments.
  enum Api {
    UNSPECIFIED,    /*!< Search for a working compiled API. */
    MACOSX_CORE,    /*!< Macintosh OS-X Core Midi API. */
    LINUX_ALSA,     /*!< The Advanced Linux Sound Architecture API. */
    UNIX_JACK,      /*!< The JACK Low-Latency MIDI Server API. */
    WINDOWS_MM,     /*!< The Microsoft Multimedia MIDI API. */
    RTMIDI_DUMMY    /*!< A compilable but non-functional API. */
  };

  //! A static function to determine the current RtMidi version.
  static std::string getVersion( void ) throw();

  //! A static function to determine the available compiled MIDI APIs.
  /*!
    The values returned in the std::vector can be compared against
    the enumerated list values.  Note that there can be more than one
    API compiled for certain operating systems.
  */
  static void getCompiledApi( std::vector<RtMidi::Api> &apis ) throw();

 protected:
  MidiApi *rtapi_;

  RtMidi();
  virtual ~RtMidi();

};

/**********************************************************************/
/*! \class RtMidiIn
    \brief A realtime MIDI input class.

    This class provides a common, platform-independent API for
    realtime MIDI input.  It allows access to a single MIDI input
    port.  Incoming MIDI messages are either saved to a queue for
    retrieval using the getMessage() function or immediately passed to
    a user-specified callback function.  Create multiple instances of
    this class to connect to more than one MIDI device at the same
    time.  With the OS-X, Linux ALSA, and JACK MIDI APIs, it is also
    possible to open a virtual input port to which other MIDI software
    clients can connect.

    by Gary P. Scavone, 2003-2014.
*/
/**********************************************************************/

// **************************************************************** //
//
// RtMidiIn and RtMidiOut class declarations.
//
// RtMidiIn / RtMidiOut are "controllers" used to select an available
// MIDI input or output interface.  They present common APIs for the
// user to call but all functionality is implemented by the classes
// MidiInApi, MidiOutApi and their subclasses.  RtMidiIn and RtMidiOut
// each create an instance of a MidiInApi or MidiOutApi subclass based
// on the user's API choice.  If no choice is made, they attempt to
// make a "logical" API selection.
//
// **************************************************************** //

class RtMidiIn : public RtMidi
{
 public:

  //! User callback function type definition.
  typedef void (*RtMidiCallback)( double timeStamp, std::vector<unsigned char> *message, void *userData);

  //! Default constructor that allows an optional api, client name and queue size.
  /*!
    An exception will be thrown if a MIDI system initialization
    error occurs.  The queue size defines the maximum number of
    messages that can be held in the MIDI queue (when not using a
    callback function).  If the queue size limit is reached,
    incoming messages will be ignored.

    If no API argument is specified and multiple API support has been
    compiled, the default order of use is ALSA, JACK (Linux) and CORE,
    JACK (OS-X).

    \param api        An optional API id can be specified.
    \param clientName An optional client name can be specified. This
                      will be used to group the ports that are created
                      by the application.
    \param queueSizeLimit An optional size of the MIDI input queue can be specified.
  */
  RtMidiIn( RtMidi::Api api=UNSPECIFIED,
            const std::string clientName = std::string( "RtMidi Input Client"),
            unsigned int queueSizeLimit = 100 );

  //! If a MIDI connection is still open, it will be closed by the destructor.
  ~RtMidiIn ( void ) throw();

  //! Returns the MIDI API specifier for the current instance of RtMidiIn.
  RtMidi::Api getCurrentApi( void ) throw();

  //! Create a virtual input port, with optional name, to allow software connections (OS X, JACK and ALSA only).
  /*!
    This function creates a virtual MIDI input port to which other
    software applications can connect.  This type of functionality
    is currently only supported by the Macintosh OS-X, any JACK,
    and Linux ALSA APIs (the function returns an error for the other APIs).

    \param portName An optional name for the applicaction port that is
           used to connect to portId can be specified.
  */
  void openVirtualPort( const std::string portName = std::string( "RtMidi Input" ) );

  //! Open a MIDI input connection given by enumeration number.
  /*!
    \param portNumber An optional port number greater than 0 can be specified.
                      Otherwise, the default or first port found is opened.
    \param portName An optional name for the application port that is used to connect to portId can be specified.
  */
  void openPort( unsigned int portNumber = 0, const std::string portName = std::string( "RtMidi Input" ) );

  //! Open a MIDI input connection given by a port descriptor.
  /*!
    \param port     A port descriptor of the port must be specified.
    \param portName An optional name for the applicaction port that is used to connect to portId can be specified.
  */
  void openPort( const PortDescriptor & port, const std::string portName = std::string( "RtMidi" ) );

  //! Returns a port descirptor if the port is open
  virtual CommonMidiApi::PortDescriptor::Pointer getDescriptor();

  //! Return a list of all available input ports of the current API.
  /*!
    \param capabilities an optional parameter that describes which
    device types are returned.
    \return This function returns a list of port descriptors.
    \note An Output API is not required to return all output ports from RtMidiIn.
  */
  virtual CommonMidiApi::PortList getPortList(int capabilities = PortDescriptor:: INPUT);

  //! Return the number of available MIDI input ports.
  /*!
    \return This function returns the number of midi ports of the selected API.
  */
  unsigned int getPortCount();

  //! Return a string identifier for the specified MIDI input port number.
  /*!
    \return The name of the port with the given Id is returned.
    \retval An empty string is returned if an invalid port specifier is provided.
  */
  std::string getPortName( unsigned int portNumber = 0 );

  //! Close an open MIDI connection (if one exists).
  void closePort( void );

  //! Returns true if a port is open and false if not.
  virtual bool isPortOpen() const;

  //! Set a callback function to be invoked for incoming MIDI messages.
  /*!
    The callback function will be called whenever an incoming MIDI
    message is received.  While not absolutely necessary, it is best
    to set the callback function before opening a MIDI port to avoid
    leaving some messages in the queue.

    \param callback A callback function must be given.
    \param userData Optionally, a pointer to additional data can be
                    passed to the callback function whenever it is called.
  */
  void setCallback( RtMidiCallback callback, void *userData = 0 );

  //! Cancel use of the current callback function (if one exists).
  /*!
    Subsequent incoming MIDI messages will be written to the queue
    and can be retrieved with the \e getMessage function.
  */
  void cancelCallback();

  //! Specify whether certain MIDI message types should be queued or ignored during input.
  /*!
    By default, MIDI timing and active sensing messages are ignored
    during message input because of their relative high data rates.
    MIDI sysex messages are ignored by default as well.  Variable
    values of "true" imply that the respective message type will be
    ignored.
  */
  void ignoreTypes( bool midiSysex = true, bool midiTime = true, bool midiSense = true );

  //! Fill the user-provided vector with the data bytes for the next available MIDI message in the input queue and return the event delta-time in seconds.
  /*!
    This function returns immediately whether a new message is
    available or not.  A valid message is indicated by a non-zero
    vector size.  An exception is thrown if an error occurs during
    message retrieval or an input connection was not previously
    established.
  */
  double getMessage( std::vector<unsigned char> *message );

  //! Set an error callback function to be invoked when an error has occured.
  /*!
    The callback function will be called whenever an error has occured. It is best
    to set the error callback function before opening a port.
  */
  virtual void setErrorCallback( RtMidiErrorCallback errorCallback = NULL );

 protected:
  void openMidiApi( RtMidi::Api api, const std::string clientName, unsigned int queueSizeLimit );

};

/**********************************************************************/
/*! \class RtMidiOut
    \brief A realtime MIDI output class.

    This class provides a common, platform-independent API for MIDI
    output.  It allows one to probe available MIDI output ports, to
    connect to one such port, and to send MIDI bytes immediately over
    the connection.  Create multiple instances of this class to
    connect to more than one MIDI device at the same time.  With the
    OS-X, Linux ALSA and JACK MIDI APIs, it is also possible to open a
    virtual port to which other MIDI software clients can connect.

    by Gary P. Scavone, 2003-2014.
*/
/**********************************************************************/

class RtMidiOut : public RtMidi
{
 public:

  //! Default constructor that allows an optional client name.
  /*!
    An exception will be thrown if a MIDI system initialization error occurs.

    If no API argument is specified and multiple API support has been
    compiled, the default order of use is ALSA, JACK (Linux) and CORE,
    JACK (OS-X).
  */
  RtMidiOut( RtMidi::Api api=UNSPECIFIED,
             const std::string clientName = std::string( "RtMidi Output Client") );

  //! The destructor closes any open MIDI connections.
  ~RtMidiOut( void ) throw();

  //! Returns the MIDI API specifier for the current instance of RtMidiOut.
  RtMidi::Api getCurrentApi( void ) throw();

  //! Create a virtual output port, with optional name, to allow software connections (OS X and ALSA only).
  /*!
      This function creates a virtual MIDI output port to which other
      software applications can connect.  This type of functionality
      is currently only supported by the Macintosh OS-X and Linux ALSA
      APIs (the function does nothing with the other APIs).  An
      exception is thrown if an error occurs while attempting to create
      the virtual port.
  */
  void openVirtualPort( const std::string portName = std::string( "RtMidi Output" ) );

  //! Open a MIDI output connection given by an enumeration number.
  /*!
      An optional port number greater than 0 can be specified.
      Otherwise, the default or first port found is opened.  An
      exception is thrown if an error occurs while attempting to make
      the port connection.
  */
  void openPort( unsigned int portNumber = 0, const std::string portName = std::string( "RtMidi Output" ) );

  //! Open a MIDI output connection given by a port descriptor.
  /*!
    \param port     A port descriptor of the port must be specified.
    \param portName An optional name for the applicaction port that is used to connect to portId can be specified.
  */
  virtual void openPort( const PortDescriptor & port, const std::string portName = std::string( "RtMidi Output" ) );

  //! Returns a port descirptor if the port is open
  /*! \return Port descriptor of the currently open port
   *  \retval 0 iff the port s not open
   */
  virtual CommonMidiApi::PortDescriptor::Pointer getDescriptor();

  //! Close an open MIDI connection (if one exists).
  void closePort( void );

  //! Returns true if a port is open and false if not.
  virtual bool isPortOpen() const;

  //! Return a list of all available ports of the current API.
  /*!
    \param capabilities an opitonnal parameter that describes which
    device types are returned.
    \return This function returns a list of port descriptors.
    \note An API is not required to return all input ports from RtMidiOut.
  */
  virtual CommonMidiApi::PortList getPortList(int capabilities = PortDescriptor::OUTPUT);

  //! Return the number of available MIDI output ports.
  unsigned int getPortCount( void );

  //! Return a string identifier for the specified MIDI port type and number.
  /*!
    \return The name of the port with the given Id is returned.
    \retval An empty string is returned if an invalid port specifier is provided.
  */
  std::string getPortName( unsigned int portNumber = 0 );

  //! Immediately send a single message out an open MIDI output port.
  /*!
      An exception is thrown if an error occurs during output or an
      output connection was not previously established.
  */
  void sendMessage( std::vector<unsigned char> *message );

  //! Set an error callback function to be invoked when an error has occured.
  /*!
    The callback function will be called whenever an error has occured. It is best
    to set the error callback function before opening a port.
  */
  virtual void setErrorCallback( RtMidiErrorCallback errorCallback = NULL );

 protected:
  void openMidiApi( RtMidi::Api api, const std::string clientName );
};


// **************************************************************** //
//
// MidiInApi / MidiOutApi class declarations.
//
// Subclasses of MidiInApi and MidiOutApi contain all API- and
// OS-specific code necessary to fully implement the RtMidi API.
//
// Note that MidiInApi and MidiOutApi are abstract base classes and
// cannot be explicitly instantiated.  RtMidiIn and RtMidiOut will
// create instances of a MidiInApi or MidiOutApi subclass.
//
// **************************************************************** //

class MidiApi: public CommonMidiApi
{
 public:

  MidiApi();
  virtual ~MidiApi();

  virtual unsigned int getPortCount( void ) = 0;
  virtual std::string getPortName( unsigned int portNumber ) = 0;

  bool isPortOpen() const { return connected_; }
  void setErrorCallback( RtMidiErrorCallback errorCallback );

  //! A basic error reporting function for RtMidi classes.
  void error( RtMidiError::Type type, std::string errorString );

protected:
  virtual void initialize( const std::string& clientName ) = 0;

  void *apiData_;
  bool connected_;
  std::string errorString_;
  RtMidiErrorCallback errorCallback_;
};

class MidiInApi : public MidiApi
{
 public:

  MidiInApi( unsigned int queueSizeLimit );
  virtual ~MidiInApi( void );
  void setCallback( RtMidiIn::RtMidiCallback callback, void *userData );
  void cancelCallback( void );
  virtual void ignoreTypes( bool midiSysex, bool midiTime, bool midiSense );
  double getMessage( std::vector<unsigned char> *message );

  // A MIDI structure used internally by the class to store incoming
  // messages.  Each message represents one and only one MIDI message.
  struct MidiMessage {
    std::vector<unsigned char> bytes;
    double timeStamp;

    // Default constructor.
  MidiMessage()
  :bytes(0), timeStamp(0.0) {}
  };

  struct MidiQueue {
    unsigned int front;
    unsigned int back;
    unsigned int size;
    unsigned int ringSize;
    MidiMessage *ring;

    // Default constructor.
  MidiQueue()
  :front(0), back(0), size(0), ringSize(0) {}
  };

  // The RtMidiInData structure is used to pass private class data to
  // the MIDI input handling function or thread.
  struct RtMidiInData {
    MidiQueue queue;
    MidiMessage message;
    unsigned char ignoreFlags;
    bool doInput;
    bool firstMessage;
    void *apiData;
    bool usingCallback;
    RtMidiIn::RtMidiCallback userCallback;
    void *userData;
    bool continueSysex;

    // Default constructor.
  RtMidiInData()
  : ignoreFlags(7), doInput(false), firstMessage(true),
      apiData(0), usingCallback(false), userCallback(0), userData(0),
      continueSysex(false) {}
  };

 protected:
  RtMidiInData inputData_;
};

class MidiOutApi : public MidiApi
{
 public:

  MidiOutApi( void );
  virtual ~MidiOutApi( void );
  virtual void sendMessage( std::vector<unsigned char> *message ) = 0;
};

// **************************************************************** //
//
// Inline RtMidiIn and RtMidiOut definitions.
//
// **************************************************************** //

inline RtMidi::Api RtMidiIn :: getCurrentApi( void ) throw() { return rtapi_->getCurrentApi(); }
inline void RtMidiIn :: openPort( unsigned int portNumber, const std::string portName ) { rtapi_->openPort( portNumber, portName ); }
inline void RtMidiIn :: openVirtualPort( const std::string portName ) { rtapi_->openVirtualPort( portName ); }
inline void RtMidiIn :: closePort( void ) { rtapi_->closePort(); }
inline bool RtMidiIn :: isPortOpen() const { return rtapi_->isPortOpen(); }
inline void RtMidiIn :: setCallback( RtMidiCallback callback, void *userData ) { ((MidiInApi *)rtapi_)->setCallback( callback, userData ); }
inline void RtMidiIn :: cancelCallback( void ) { ((MidiInApi *)rtapi_)->cancelCallback(); }
inline unsigned int RtMidiIn :: getPortCount( void ) { return rtapi_->getPortCount(); }
inline std::string RtMidiIn :: getPortName( unsigned int portNumber ) { return rtapi_->getPortName( portNumber ); }
inline void RtMidiIn :: ignoreTypes( bool midiSysex, bool midiTime, bool midiSense ) { ((MidiInApi *)rtapi_)->ignoreTypes( midiSysex, midiTime, midiSense ); }
inline double RtMidiIn :: getMessage( std::vector<unsigned char> *message ) { return ((MidiInApi *)rtapi_)->getMessage( message ); }
inline void RtMidiIn :: setErrorCallback( RtMidiErrorCallback errorCallback ) { rtapi_->setErrorCallback(errorCallback); }

inline RtMidi::Api RtMidiOut :: getCurrentApi( void ) throw() { return rtapi_->getCurrentApi(); }
inline void RtMidiOut :: openPort( unsigned int portNumber, const std::string portName ) { rtapi_->openPort( portNumber, portName ); }
inline void RtMidiOut :: openVirtualPort( const std::string portName ) { rtapi_->openVirtualPort( portName ); }
inline void RtMidiOut :: closePort( void ) { rtapi_->closePort(); }
inline bool RtMidiOut :: isPortOpen() const { return rtapi_->isPortOpen(); }
inline unsigned int RtMidiOut :: getPortCount( void ) { return rtapi_->getPortCount(); }
inline std::string RtMidiOut :: getPortName( unsigned int portNumber ) { return rtapi_->getPortName( portNumber ); }
inline void RtMidiOut :: sendMessage( std::vector<unsigned char> *message ) { ((MidiOutApi *)rtapi_)->sendMessage( message ); }
inline void RtMidiOut :: setErrorCallback( RtMidiErrorCallback errorCallback ) { rtapi_->setErrorCallback(errorCallback); }

// **************************************************************** //
//
// MidiInApi and MidiOutApi subclass prototypes.
//
// **************************************************************** //

#if !defined(__LINUX_ALSA__) && !defined(__UNIX_JACK__) && !defined(__MACOSX_CORE__) && !defined(__WINDOWS_MM__)
  #define __RTMIDI_DUMMY__
#endif

#if defined(__MACOSX_CORE__)

class MidiInCore: public MidiInApi
{
 public:
  MidiInCore( const std::string clientName, unsigned int queueSizeLimit );
  ~MidiInCore( void );
  RtMidi::Api getCurrentApi( void ) { return RtMidi::MACOSX_CORE; };
  void openPort( unsigned int portNumber, const std::string portName );
  void openVirtualPort( const std::string portName );
  void openPort( const PortDescriptor & port, const std::string portName);
  CommonMidiApi::PortDescriptor::Pointer getDescriptor();
  CommonMidiApi::PortList getPortList(int capabilities);
  void closePort( void );
  unsigned int getPortCount( void );
  std::string getPortName( unsigned int portNumber );

 protected:
  void initialize( const std::string& clientName );
};

class MidiOutCore: public MidiOutApi
{
 public:
  MidiOutCore( const std::string clientName );
  ~MidiOutCore( void );
  RtMidi::Api getCurrentApi( void ) { return RtMidi::MACOSX_CORE; };
  void openPort( unsigned int portNumber, const std::string portName );
  void openVirtualPort( const std::string portName );
  void openPort( const PortDescriptor & port, const std::string portName);
  CommonMidiApi::PortDescriptor::Pointer getDescriptor();
  CommonMidiApi::PortList getPortList(int capabilities);
  void closePort( void );
  unsigned int getPortCount( void );
  std::string getPortName( unsigned int portNumber );
  void sendMessage( std::vector<unsigned char> *message );

 protected:
  void initialize( const std::string& clientName );
};

#endif

#if defined(__UNIX_JACK__)

class MidiInJack: public MidiInApi
{
 public:
  MidiInJack( const std::string clientName, unsigned int queueSizeLimit );
  ~MidiInJack( void );
  RtMidi::Api getCurrentApi( void ) { return RtMidi::UNIX_JACK; };
  void openPort( unsigned int portNumber, const std::string portName );
  void openVirtualPort( const std::string portName );
  void openPort( const PortDescriptor & port, const std::string portName);
  CommonMidiApi::PortDescriptor::Pointer getDescriptor();
  CommonMidiApi::PortList getPortList(int capabilities);
  void closePort( void );
  unsigned int getPortCount( void );
  std::string getPortName( unsigned int portNumber );

 protected:
  std::string clientName;

  void connect( void );
  void initialize( const std::string& clientName );
};

class MidiOutJack: public MidiOutApi
{
 public:
  MidiOutJack( const std::string clientName );
  ~MidiOutJack( void );
  RtMidi::Api getCurrentApi( void ) { return RtMidi::UNIX_JACK; };
  void openPort( unsigned int portNumber, const std::string portName );
  void openVirtualPort( const std::string portName );
  void openPort( const PortDescriptor & port, const std::string portName);
  CommonMidiApi::PortDescriptor::Pointer getDescriptor();
  CommonMidiApi::PortList getPortList(int capabilities);
  void closePort( void );
  unsigned int getPortCount( void );
  std::string getPortName( unsigned int portNumber );
  void sendMessage( std::vector<unsigned char> *message );

 protected:
  std::string clientName;

  void connect( void );
  void initialize( const std::string& clientName );
};

#endif

#if defined(__LINUX_ALSA__)

class MidiInAlsa: public MidiInApi
{
 public:
  MidiInAlsa( const std::string clientName, unsigned int queueSizeLimit );
  ~MidiInAlsa( void );
  RtMidi::Api getCurrentApi( void ) throw() { return RtMidi::LINUX_ALSA; };
  void openPort( unsigned int portNumber, const std::string portName );
  void openVirtualPort( const std::string portName );
  void openPort( const PortDescriptor & port, const std::string portName);
  CommonMidiApi::PortDescriptor::Pointer getDescriptor();
  CommonMidiApi::PortList getPortList(int capabilities);
  void closePort( void );
  unsigned int getPortCount( void );
  std::string getPortName( unsigned int portNumber );

 protected:
  void initialize( const std::string& clientName );
};

class MidiOutAlsa: public MidiOutApi
{
 public:
  MidiOutAlsa( const std::string clientName );
  ~MidiOutAlsa( void );
  RtMidi::Api getCurrentApi( void ) throw() { return RtMidi::LINUX_ALSA; };
  void openPort( unsigned int portNumber, const std::string portName );
  void openVirtualPort( const std::string portName );
  void openPort( const PortDescriptor & port, const std::string portName);
  CommonMidiApi::PortDescriptor::Pointer getDescriptor();
  CommonMidiApi::PortList getPortList(int capabilities);
  void closePort( void );
  unsigned int getPortCount( void );
  std::string getPortName( unsigned int portNumber );
  void sendMessage( std::vector<unsigned char> *message );

 protected:
  void initialize( const std::string& clientName );
};

#endif

#if defined(__WINDOWS_MM__)

class MidiInWinMM: public MidiInApi
{
 public:
  MidiInWinMM( const std::string clientName, unsigned int queueSizeLimit );
  ~MidiInWinMM( void );
  RtMidi::Api getCurrentApi( void ) { return RtMidi::WINDOWS_MM; };
  void openPort( unsigned int portNumber, const std::string portName );
  void openVirtualPort( const std::string portName );
  void openPort( const PortDescriptor & port, const std::string portName);
  CommonMidiApi::PortDescriptor::Pointer getDescriptor();
  CommonMidiApi::PortList getPortList(int capabilities);
  void closePort( void );
  unsigned int getPortCount( void );
  std::string getPortName( unsigned int portNumber );

 protected:
  void initialize( const std::string& clientName );
};

class MidiOutWinMM: public MidiOutApi
{
 public:
  MidiOutWinMM( const std::string clientName );
  ~MidiOutWinMM( void );
  RtMidi::Api getCurrentApi( void ) { return RtMidi::WINDOWS_MM; };
  void openPort( unsigned int portNumber, const std::string portName );
  void openVirtualPort( const std::string portName );
  void openPort( const PortDescriptor & port, const std::string portName);
  CommonMidiApi::PortDescriptor::Pointer getDescriptor();
  CommonMidiApi::PortList getPortList(int capabilities);
  void closePort( void );
  unsigned int getPortCount( void );
  std::string getPortName( unsigned int portNumber );
  void sendMessage( std::vector<unsigned char> *message );

 protected:
  void initialize( const std::string& clientName );
};

#endif

#if defined(__RTMIDI_DUMMY__)

class MidiInDummy: public MidiInApi
{
 public:
  MidiInDummy( const std::string /*clientName*/, unsigned int queueSizeLimit )
	  : MidiInApi( queueSizeLimit ) {
	  errorString_ = "MidiInDummy: This class provides no functionality.";
	  error( RtMidiError::WARNING, errorString_ );
  }
  RtMidi::Api getCurrentApi( void ) { return RtMidi::RTMIDI_DUMMY; }
  void openPort( unsigned int /*portNumber*/, const std::string /*portName*/ ) {}
  void openVirtualPort( const std::string /*portName*/ ) {}
  void openPort( const PortDescriptor & port, const std::string portName) {}
  CommonMidiApi::PortDescriptor::Pointer getDescriptor() { return 0; }
  CommonMidiApi::PortList getPortList(int capabilities) { return CommonMidiApi::PortList(); }
  void closePort( void ) {}
  unsigned int getPortCount( void ) { return 0; }
  std::string getPortName( unsigned int portNumber ) { return ""; }

 protected:
  void initialize( const std::string& /*clientName*/ ) {}
};

class MidiOutDummy: public MidiOutApi
{
 public:
  MidiOutDummy( const std::string /*clientName*/ ) {
	  errorString_ = "MidiOutDummy: This class provides no functionality.";
	  error( RtMidiError::WARNING, errorString_ );
  }
  RtMidi::Api getCurrentApi( void ) { return RtMidi::RTMIDI_DUMMY; }
  void openPort( unsigned int /*portNumber*/, const std::string /*portName*/ ) {}
  void openVirtualPort( const std::string /*portName*/ ) {}
  void openPort( const PortDescriptor & port, const std::string portName) {}
  CommonMidiApi::PortDescriptor::Pointer getDescriptor() { return 0; }
  CommonMidiApi::PortList getPortList(int capabilities) { return CommonMidiApi::PortList(); }
  void closePort( void ) {}
  unsigned int getPortCount( void ) { return 0; }
  std::string getPortName( unsigned int /*portNumber*/ ) { return ""; }
  void sendMessage( std::vector<unsigned char> * /*message*/ ) {}

 protected:
  void initialize( const std::string& /*clientName*/ ) {}
};

#endif

#endif
