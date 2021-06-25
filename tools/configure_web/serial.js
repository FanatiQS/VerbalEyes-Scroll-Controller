// Creates a serial device connection for reading and writing
function SerialDevice() {
	// Make sure Web Serial is supported
	if (!navigator.serial) {
		alert("Web Serial is not supported");
		throw new Error("Web Serial is not supported");
	}

	// Connectss to serial device
	return navigator.serial.requestPort().then(async (port) => {
		await port.open({ baudRate: 9600 });

		// Sets up and resolves to the instance
		this.port = port;
		this.reader = port.readable.pipeThrough(new TextDecoderStream()).getReader();
		this.writer = port.writable.getWriter();
		return this;
	});
}

// Writes a string to the serial devicce
SerialDevice.prototype.write = function (str) {
	this.writer.write(new TextEncoder().encode(str));
};

// Asynchronously get logs from device
SerialDevice.prototype[Symbol.asyncIterator] = function () {
	return { next: () => this.reader.read() };
};

// Close device connection
SerialDevice.prototype.close = async function () {
	await this.reader.cancel();
	await this.writer.close();
	await this.port.close();
};
