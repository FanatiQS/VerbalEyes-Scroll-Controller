async function getSerialDevice() {
	// Connects to serial device
	const port = await navigator.serial.requestPort();
	await port.open({baudrate: 9600});

	// Creates write stream that automatically encodes the string to an arraybuffer
	const stream = new TextEncoderStream();
	stream.readable.pipeTo(port.writable);
	const writer = stream.writable.getWriter();

	// Creates reader stream that automatically decoded the arraybuffer to a string
	const reader = port.readable.pipeThrough(new TextDecoderStream()).getReader();

	// Makes reader an asyncIterator on the writer
	writer[Symbol.asyncIterator] = () => reader;
	reader.next = reader.read;

	// Closes reader and port when writer is closed
	writer.closed.then(async () => {
		await reader.cancel();
		port.close();
	});

	// Returns writer that also contains reader
	return writer;
};
