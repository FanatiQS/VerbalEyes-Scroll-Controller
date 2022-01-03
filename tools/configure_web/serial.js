/**
 * @TODO add documentation
 */
function SerialDevice(callback) {
	this.port = null;
	this.reader = null;
	this.printCallback = callback;
}

/**
 * Gets boolean indicating if the serial port is connected or not
 * @TODO add documentation
 */
SerialDevice.prototype.isOpen = function () {
	return this.port != null;
};

/**
 * @TODO add documentation
 * @TODO should there be an optional argument to set another baudeRate?
 */
SerialDevice.prototype.connect = async function () {
	if (!navigator.serial) throw new Error("WebSerial is not supported");
	this.port = await navigator.serial.requestPort();
	await this.port.open({ baudRate: 9600 });
};

/**
 * @TODO add documentation
 */
SerialDevice.prototype.close = async function () {
	if (this.reader) await this.stopReading();
	await this.port.close();
	this.port = null;
};

/**
 * @TODO add documentation
 */
SerialDevice.prototype[Symbol.asyncIterator] = function () {
	return this;
};

/**
 * @TODO add documentation
 */
SerialDevice.prototype.next = function () {
	return new Promise((resolve) => {
		this.printCallback = (value, done) => resolve({ value, done });
	});
};

/**
 * @TODO add documentation
 * @TODO document that it resolves when reading stops, does not have to await
 */
SerialDevice.prototype.startReading = async function () {
	// Closes existing reader if available
	if (this.reader) await this.reader.cancel();

	// Creates new reader
	this.reader = this.port.readable.getReader();

	// Flushes data until CRLF to prevent printing half messages
	await this.reader.read();
	while (this.reader) {
		const data = await this.reader.read();
		if (data.done) return;
		const chunk = String.fromCharCode(...data.value);
		const index = chunk.indexOf('\n');
		if (index !== -1) {
			this.printCallback(chunk.slice(index + 1));
			break;
		}
	}

	// Prints all data read
	while (this.reader) {
		const data = await this.reader.read();
		if (data.done) return;
		this.printCallback(String.fromCharCode(...data.value));
	}
};

/**
 * @TODO add documentation
 */
SerialDevice.prototype.stopReading = async function () {
	await this.reader.cancel();
	this.reader = null;
};

/**
 * @TODO add documentation
 */
SerialDevice.prototype.setReading = function (state) {
	return (state) ? this.startReading() : this.stopReading();
};

/**
 * Serializes an iterable list of HTML nodes
 * @TODO add documentation
 * @TODO should this be a method or a function?
 * @TODO is it faster to create an array to join or to just loop over nodes list and create string right away?
 */
SerialDevice.prototype.serialize = function (nodes) {
	return Array.from(nodes, (node) => `${node.name}=${node.value}\n`).join('') + '\n';
};

/**
 * @TODO add documentation
 * @TODO maybe create buffer before writing data to ensure no response for the write is trimmed
 */
SerialDevice.prototype.write = async function (data) {
	// Creates writer for serial device
	const writer = this.port.writable.getWriter();

	// Only writes data if a reader already exists
	if (this.reader) {
		await writer.write(new TextEncoder().encode(data));
		await writer.close();
	}
	// Creates reader and writes data if no reader is available
	else {
		// Creates reader and flushes first read
		this.reader = this.port.readable.getReader();
		await this.reader.read();

		// Writes data
		const writeProgress = writer.write(new TextEncoder().encode(data));

		// Flushes data until configuration logs
		let buffer = "";
		while (true) {
			const data = await this.reader.read();
			if (data.done) {
				await writeProgress;
				await writer.close();
			}
			const chunk = String.fromCharCode(...data.value);
			const index = chunk.indexOf('[');
			if (index !== -1 && (index === 0 || chunk[index - 1] === '\n')) {
				buffer = chunk.slice(index);
				break;
			}
		}

		// Prints configuration logs and breaks after completed
		while (true) {
			const data = await this.reader.read();
			buffer += String.fromCharCode(...data.value);
			const index = buffer.indexOf('\n');
			const chunk = buffer.slice(0, index + 1);
			buffer = buffer.slice(index + 1);
			if (
				chunk.includes("Configuration saved\r\n") ||
				chunk.includes("Configureation canceled\r\n")
			) {
				this.printCallback(chunk.slice(0, chunk.indexOf('\n') + 1));
				break;
			}
			else {
				this.printCallback(chunk);
			}
		}

		// Cleans up reader and writer
		await this.reader.cancel();
		this.reader = null;
		await writeProgress;
		await writer.close();
	}
};
