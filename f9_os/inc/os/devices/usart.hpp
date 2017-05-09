#ifndef _DEVICE_USART_H_
#define _DEVICE_USART_H_


#include "../types.hpp"

#include "../driver.hpp"
#include "../thread.hpp"
#include <scm/usrlib.h>
namespace os {
	namespace device {
	// wrapper between uart and the driver
		enum class uart_trasfer_type {
			blocking,
			dma,
			isr
		};
		struct uart_device {
			virtual bool init(void* args); // basicly open
			virtual void trasmit_start()=0;
			virtual void trasmit_stop()=0;
			virtual bool trasmitting() const=0;
			virtual void recive_start()=0;
			virtual void recive_stop()=0;
			virtual bool reciving() const = 0;
			virtual void recive_data(int c)=0;
			virtual int trasmit_next()=0;
			virtual void trasmit_complete()=0;
			// blocking put and get
			virtual void write(uint8_t c);
			virtual int  read();
			virtual ~uart_device() {}
		};
		class uart :  public device_driver  {
		protected:
			enum uart_state {
				uart_async_stopped = 0,
				uart_async_tramitting = (1<<1),
				uart_async_reciving = (1<<2),
				uart_async_error = (1<<3),
			};
			static constexpr size_t INCOMMING_BUFFER_SIZE = 128;
			static constexpr size_t OUTGOING_BUFFER_SIZE = 128;
			usr::TCbuf _incomming;
			usr::TCbuf _outgoing;
			// alligned for dma trasfers
			__attribute__ ((aligned(4))) uint8_t _incomming_buffer[INCOMMING_BUFFER_SIZE];
			__attribute__ ((aligned(4))) uint8_t _outgoing_buffer[OUTGOING_BUFFER_SIZE];

			uart_trasfer_type _type;
			uart_state _state;
			bool _incomming_wait;
					bool _outgoing_wait;
			// commands from uart_device
			// return
			void recive_data(int c){  // called from the isr when we get a byte
				if(_incomming.get_count() >= OUTGOING_BUFFER_SIZE) return; // overflow?
				_incomming.put(c);
				if(_incomming.get_count() == OUTGOING_BUFFER_SIZE) {
					isr_trasmit_stop();
				}
				if(_incomming_wait) {
					_incomming_wait = false;
					kernel::wakeup(&_incomming_wait);
				}
			}
			int isr_trasmiter_next_byte(){
				if(_outgoing.get_count() > 0) {
					uint8_t data = _outgoing.get();
					return data;
				} else return -1;
				; // called when trasmiter needs another byte, -1 if complete
				// it will then call isr_trasmit_complete on this complete send
			}
			void isr_trasmit_complete(){ // complete trasmition from isr
				// trasmit should be stopped here
			  if(_outgoing.get_count() > 0) isr_recive_start(); // restart if we have data
			  if(_outgoing_wait){
				  _outgoing_wait = false;
					kernel::wakeup(&_outgoing_wait);
			  }
			}
			void isr_error(int error) {
				assert(error==0); // just freeze for right now
				// called when in error
			}

			virtual void isr_trasmit_start() = 0;
			virtual void isr_trasmit_stop() = 0;
			virtual bool isr_trasmitting() const = 0;
			virtual void isr_recive_start() = 0;
			virtual void isr_recive_stop() = 0;
			virtual bool isr_reciving() const = 0;
			virtual bool usart_init(void* args)  = 0;
			virtual void blocking_write(int c)  = 0;
			virtual int blocking_read()  = 0;
		public:
			uart(uart_trasfer_type type = uart_trasfer_type::blocking) : _incomming(_incomming_buffer, sizeof(_incomming_buffer)) ,
					_outgoing(_outgoing_buffer, sizeof(_outgoing_buffer)),_type(type) , _state(uart_async_stopped) ,_incomming_wait(false), _outgoing_wait(false) {}
			uart_trasfer_type trasfer_type() const { return _type; }

			void start() {
				if(_type != uart_trasfer_type::blocking){
					if(_outgoing.get_count() > 0 && !isr_trasmitting())
						isr_trasmit_start();
					if(_incomming.get_count() < OUTGOING_BUFFER_SIZE && !isr_reciving())
						isr_recive_start();

				}
			}
			void stop() {
				if(_type != uart_trasfer_type::blocking){
					isr_trasmit_stop();
					isr_recive_stop();
				}
			}
			// interface
			bool open(open_mode mode,void* args) override{
				stop();
				if(!usart_init(args)) return false;
				start();
				return true;
			}
			bool isopen() const override { return true; }  // always open
			size_t write(const void *src, size_t bytes) override {
				const uint8_t* data = static_cast<const uint8_t*>(src);
				if(_type == uart_trasfer_type::blocking){
					while(bytes--) blocking_write(*data++);
				} else {
					while(bytes--){
						while(_outgoing.put(*data) == false) {
							_outgoing_wait = true;
							kernel::sleep(&_outgoing_wait, kernel::PWAIT);
						}
						data++;
					}
					start();
				}
				return 0;
			}
			size_t read(void *dst, size_t bytes) override {
				uint8_t* data = static_cast<uint8_t*>(dst);
				if(_type == uart_trasfer_type::blocking){
					while(bytes--) *data++ = static_cast<uint8_t>(blocking_read());
				} else {
					while(bytes--){
						while(_incomming.get_count() == 0) {
							_incomming_wait = true;
							kernel::sleep(&_incomming_wait, kernel::PWAIT);
						}
						*data++ = _incomming.get();
					}
				}
				return 0;
			}
			bool close() override { return true; }
			size_t ioctrl(int flags, int mode, void* args) override { return 0; }
		};


	};
};





#endif
