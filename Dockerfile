# Use an official GCC image as a parent image
FROM gcc:latest

# Set the working directory in the container
WORKDIR /app

# Copy the server and client directories to the container
COPY server/ /app/server/
COPY client/ /app/client/

# Compile the server
RUN make -C /app/server

# Create the logs directory
RUN mkdir -p /app/server/logs

# Expose the port the server runs on
EXPOSE 8080

# Run the server when the container launches
CMD ["/app/server/ftp_server"]
