/*
 * Copyright (c) 2011 Dan Wilcox <danomatika@gmail.com>
 *
 * BSD Simplified License.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * See https://github.com/danomatika/ofxPd for documentation
 *
 * This project uses libpd, copyrighted by Miller Puckette and others using the
 * "Standard Improved BSD License". See the file "LICENSE.txt" in src/pd.
 *
 * See http://gitorious.org/pdlib/pages/Libpd for documentation
 *
 */
#pragma once

#include <map>
#include <set>
#include <Poco/Mutex.h>

#include "z_libpd.h"
#include "PdReceiver.h"

#ifndef HAVE_UNISTD_H
#warning You need to define HAVE_UNISTD_H in your project build settings!
#endif

///
///	a Pure Data instance
///
///	references:	http://gitorious.org/pdlib/pages/Libpd
///
/// note: libpd currently does not support multiple states and it is 
///       suggested that you use only one ofxPd object at a time
///
class ofxPd {
	
	public :

		ofxPd();
		virtual ~ofxPd();
        
        /// \section Initializing Pd
        
        /// initialize resources
		///
		/// set the audio latency by setting the libpd ticks per buffer:
		/// ticks per buffer * lib pd block size (always 64)
		///
		/// ie 4 ticks per buffer * 64 = buffer len of 512
		///
		/// the lower the number of ticks, the faster the audio processing
		/// if you experience audio dropouts (audible clicks), increase the
		/// ticks per buffer
		///
		bool init(const int numOutChannels, const int numInChannels,
				  const int sampleRate, const int ticksPerBuffer=32);
        
        /// clear resources
        void clear();
		
		/// \section Adding Search Paths
		
		/// add to the pd search path
		/// takes an absolute or relative path (in data folder)
        ///
		/// note: fails silently if path not found
		///
        void addToSearchPath(const std::string& path);
		
		/// clear the current pd search path
		void clearSearchPath();
		
		/// \section Opening Patches
		
		/// open a patch file, takes an absolute or relative path (in data folder)
		/// returns a Patch object
		pd::Patch openPatch(const std::string& patch);
		
		/// close a patch file, takes the patch's basename (filename without extension)
		void closePatch(const std::string& patch);
		
		/// close a patch file, takes a patch object
		/// clears the given Patch object
		void closePatch(pd::Patch& patch);
		
		/// \section Audio Processing Control
		
		/// start/stop audio processing
        ///
        /// note: in general, once started, you won't need to turn off audio processing
        ///
        /// shortcuts for [; pd dsp 1( & [; pd dsp 0(
        ///
		void start();
		void stop();
		
		//// \section Receiving
		
		/// subscribe/unsubscribe to source names from libpd
		///
		/// aka the pd receive name
		///
		/// [r source]
		/// |
		///
		/// note: the global source (aka "") exists by default 
		///
		void subscribe(const std::string& source);
		void unsubscribe(const std::string& source);
		bool isSubscribed(const std::string& source);
		void unsubscribeAll(); ///< listeners will be unsubscribed from *all* sources
		
        /// add/remove incoming event receiver
		///
		/// receivers automatically receive from *all* subscribed sources
        /// as well as print and midi events
        ///
        /// see receive/ignore for specific source recieving control
		///
		void addReceiver(pd::PdReceiver& receiver);
		void removeReceiver(pd::PdReceiver& receiver);
		bool receiverExists(pd::PdReceiver& receiver);
		void clearReceivers();	/// also unsubscribes all receivers
        
		/// set a reciever to receive/ignore a subscribed source from libpd
		///
		/// receive/ignore using a source name or "" for all sources,
		/// make sure to add the receiver and source first
		///
		/// note: the global source (aka "") is added by default
		/// note: ignoring the global source ignores *all* sources,
		///       so the receiver will not receive any message events,
		///		  but still get print and midi events
		///
		/// also: use negation if you want to plug into all sources but one:
		///
		/// pd.receive(receiver);			// receive from *all*
		/// pd.ignore(receiver, "source");  // ignore "source"
		///
		void receive(pd::PdReceiver& receiver, const std::string& source="");
		void ignore(pd::PdReceiver& receiver, const std::string& source="");
		bool isReceiving(pd::PdReceiver& receiver, const std::string& source="");
		
		/// \section Sending Functions
		
		/// messages
		void sendBang(const std::string& dest);
		void sendFloat(const std::string& dest, float value);
		void sendSymbol(const std::string& dest, const std::string& symbol);
		
		/// compound messages
		///
		/// pd.startMsg();
		/// pd.addSymbol("hello");
		/// pd.addFloat(1.23);
		/// pd.finishList("test");  // "test" is the reciever name in pd
		///
		/// sends [list hello 1.23( -> [r test],
        /// you will need to use the [list trim] object on the reciving end 
		/// 
        /// finishMsg sends a typed message -> [; test msg1 hello 1.23(
        ///
        /// pd.startMsg();
        /// pd.addSymbol("hello");
		/// pd.addFloat(1.23);
		/// pd.finishMsg("test", "msg1");
        ///
		void startMsg();
		void addFloat(const float value);
		void addSymbol(const std::string& symbol);
		void finishList(const std::string& dest);
        void finishMsg(const std::string& dest, const std::string& msg);
        
        /// compound messages using the ofxPd List type
        ///
        /// List list;
        /// list.addSymbol("hello");
        /// list.addFloat(1.23);
        /// pd.sendList("test", list);
        ///
        /// sends [list hello 1.23( -> [r test]
        ///
        /// clear the list:
        ///
        /// list.clear();
        ///
        /// stream operators work as well:
        ///
        /// list << "hello" << 1.23;
        /// pd.sendMsg("test", "msg1", list);
        /// 
        /// sends a typed message -> [; test msg1 hello 1.23(
        ///
        void sendList(const std::string& dest, const pd::List& list);
        void sendMsg(const std::string& dest, const std::string& msg, const pd::List& list);
		
		/// midi
		///
		/// send midi messages, any out of range messages will be silently ignored
		///
		/// number ranges:
        /// channel		1 - 16 * dev# (dev #0: 1-16, dev #1: 17-32, etc)
		/// pitch 		0 - 127
		/// velocity	0 - 127
		/// control value	0 - 127
		/// program value	1 - 128
		/// bend value		-8192 - 8191
		/// touch value		0 - 127
		///
		/// note, in pd:
        /// [bendin] takes 0 - 16383 while [bendout] returns -8192 - 8192
		/// [pgmin] and [pgmout] are 1 - 128
        ///
		void sendNote(const int channel, const int pitch, const int velocity=64);
		void sendCtl(const int channel, const int controller, const int value);
		void sendPgm(const int channel, const int value);
		void sendBend(const int channel, const int value);
		void sendTouch(const int channel, const int value);
		void sendPolyTouch(const int channel, const int pitch, const int value);		
		
		/// raw midi bytes
		///
		/// value is a raw midi byte value 0 - 255
		/// port is the raw portmidi port #, similar to a channel
		///
        /// for some reason, [midiin], [sysexin] & [realtimein] add 2 to the port num,
        /// so sending to port 1 in ofxPd returns port 3 in pd
        /// 
        /// however, [midiout], [sysexout], & [realtimeout] do not add to the port num,
        /// so sending port 1 to [midiout] returns port 1 in ofxPd
        ///
		void sendMidiByte(const int port, const int value);
		void sendSysExByte(const int port, const int value);
		void sendSysRtByte(const int port, const int value);
		
		/// \section Sending Stream Interface
		
		/// single messages
		///
		/// pd << Bang("test"); /// "test" is the reciever name in pd
		/// pd << Float("test", 100);
		/// pd << Symbol("test", "a symbol");
		///
		ofxPd& operator<<(const pd::Bang& var);
		ofxPd& operator<<(const pd::Float& var);
		ofxPd& operator<<(const pd::Symbol& var);
		
		/// compound messages
		///
		/// pd << StartMsg() << 100 << 1.2 << "a symbol" << FinishList("test");
		///
		ofxPd& operator<<(const pd::StartMsg& var);
        ofxPd& operator<<(const pd::FinishList& var);
        ofxPd& operator<<(const pd::FinishMsg& var);
        
		/// add a float to the message
		ofxPd& operator<<(const bool var);
        ofxPd& operator<<(const int var);
        ofxPd& operator<<(const float var);
        ofxPd& operator<<(const double var);
        
		/// add a symbol to the message
		ofxPd& operator<<(const char var);
        ofxPd& operator<<(const char* var);
        ofxPd& operator<<(const std::string& var);
		
		/// midi
		///
		/// pd << Note(64) << Note(64, 60) << Note(64, 60, 1);
		/// pd << Ctl(100, 64) << Pgm(100, 1) << Bend(2000, 1);
		/// pd << Touch(127, 1) << PolyTouch(64, 127, 1);
		///
		ofxPd& operator<<(const pd::Note& var);
		ofxPd& operator<<(const pd::Ctl& var);
		ofxPd& operator<<(const pd::Pgm& var);
		ofxPd& operator<<(const pd::Bend& var);
		ofxPd& operator<<(const pd::Touch& var);
		ofxPd& operator<<(const pd::PolyTouch& var);
		
		/// compound raw midi byte stream
		///
		/// pd << StartMidi() << 0xEF << 0x45 << Finish();
		/// pd << StartSysEx() << 0xE7 << 0x45 << 0x56 << 0x17 << Finish();
		///
		ofxPd& operator<<(const pd::StartMidi& var);
		ofxPd& operator<<(const pd::StartSysEx& var);
		ofxPd& operator<<(const pd::StartSysRt& var);
        ofxPd& operator<<(const pd::Finish& var);
		
		/// is a message or byte stream currently in progress?
        inline bool isMsgInProgress() {return bMsgInProgress;}
		
		/// \section Array Access
		
		/// get size of pd array
		/// returns 0 if array not found
		int getArraySize(const std::string& arrayName);
		
		/// read from a pd array
		///
		/// resizes given vector to readLen, checks readLen and offset
		///
		/// returns true on success, false on failure
		///
		/// calling without setting readLen and offset reads the whole array:
		///
		/// vector<float> array1;
		/// readArray("array1", array1); 
		///
		bool readArray(const std::string& arrayName, std::vector<float>& dest,
												int readLen=-1, int offset=0);
		
		/// write to a pd array
		///
		/// calling without setting writeLen and offset writes the whole array:
		///
		/// writeArray("array1", array1); 
		///
		bool writeArray(const std::string& arrayName, std::vector<float>& source,
													int writeLen=-1, int offset=0);
		
		/// clear array and set to a specific value
		void clearArray(const std::string& arrayName, int value=0);
		
		/// \section Utils
		
		/// get the blocksize of pd (sample length per channel)
		static int getBlockSize();
        
        /// get/set the max length of messages and lists, default: 32
        void setMaxMsgLength(unsigned int len);
        unsigned int getMaxMsgLength();
		
		/// \section Audio Processing Callbacks
		
		/// the libpd processing is done in the audioOut callback
		virtual void audioIn(float * input, int bufferSize, int nChannels);
		virtual void audioOut(float * output, int bufferSize, int nChannels);
		
    private:
	
		bool bPdInited;						///< is pd inited?

		int sampleRate;						///< the audio sample rate
		int ticksPerBuffer;					///< how many pd blocks per buffer frame
		int numInChannels, numOutChannels;	///< number of channels in/out
		float *inputBuffer;  				///< interleaved input audio buffer
		
		bool bMsgInProgress;				///< is a compound message being constructed?
        int maxMsgLen;                      ///< maximum allowed message length
        int curMsgLen;                      ///< the length of the current message
		
		/// compound message status
		enum MsgType {
			MSG,
			MIDI,
			SYSEX,
			SYSRT
		} msgType;
		
		int midiPort;   ///< target midi port
	
		/// a receiving sources's pointer and bound receivers
		struct Source {
			
			// data
			void* pointer;                          ///< source pointer
			std::set<pd::PdReceiver*> receivers;    ///< subscribed receivers

			// helper functions
			void addReceiver(pd::PdReceiver* receiver) {
				receivers.insert(receiver);
			}
			
			void removeReceiver(pd::PdReceiver* receiver) {
				std::set<pd::PdReceiver*>::iterator iter;
				iter = receivers.find(receiver);
				if(iter != receivers.end())
					receivers.erase(iter);
			}

			bool receiverExists(pd::PdReceiver* receiver) {
				if(receivers.find(receiver) != receivers.end())
					return true;
				return false;
			}
		};
			
		std::set<pd::PdReceiver*> receivers;	///< the receivers
		std::map<std::string,Source> sources;	///< subscribed sources
												///< first object always global
		
		std::string printMsg;	///< used to build a print message
		
		Poco::Mutex mutex;	///< used to lock libpd for thread safety
		
		// libpd static callback functions
		static void _print(const char* s);
				
		static void _bang(const char* source);
		static void _float(const char* source, float value);
		static void _symbol(const char* source, const char* symbol);
		
		static void _list(const char* source, int argc, t_atom* argv); 
		static void _message(const char* source, const char *symbol,
												int argc, t_atom *argv);

		static void _noteon(int channel, int pitch, int velocity);
		static void _controlchange(int channel, int controller, int value);
		static void _programchange(int channel, int value);
		static void _pitchbend(int channel, int value);
		static void _aftertouch(int channel, int value);
		static void _polyaftertouch(int channel, int pitch, int value);
		
		static void _midibyte(int port, int byte);
};
