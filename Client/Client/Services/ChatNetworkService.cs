using System;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Client.Services
{
    internal class ChatNetworkService
    {
        private TcpClient tcpClient;
        private StreamReader reader;
        private StreamWriter writer;
        private CancellationTokenSource receiveCts;

        public Action<string> MessageReceived;
        public Action<string> ConnectionStatusChanged;

        public bool IsConnected => tcpClient?.Connected ?? false;

        public async Task<bool> ConnectAsync(string ip, int port)
        {
            try
            {
                tcpClient = new TcpClient();
                await tcpClient.ConnectAsync(ip, port);

                NetworkStream networkStream = tcpClient.GetStream();
                reader = new StreamReader(networkStream, Encoding.UTF8);
                writer = new StreamWriter(networkStream, Encoding.UTF8)
                {
                    AutoFlush = true
                };

                receiveCts = new CancellationTokenSource();

                ConnectionStatusChanged?.Invoke("Connected");
                _ = StartReceiveLoopAsync(receiveCts.Token);

                return true;
            }
            catch (Exception ex)
            {
                ConnectionStatusChanged?.Invoke($"Connection failed : {ex.Message}");
                return false;
            }
        }

        public async Task SendAsync(string message)
        {
            if (writer == null)
            {
                throw new InvalidOperationException("Not connected");
            }

            await writer.WriteLineAsync(message);
        }

        public async Task StartReceiveLoopAsync(CancellationToken cancellationToken)
        {
            if (reader == null)
            {
                // throw new InvalidOperationException("Not connected");
                return;
            }

            try
            {
                while(!cancellationToken.IsCancellationRequested)
                {
                    string line = await reader.ReadLineAsync();

                    if (line == null)
                    {
                        ConnectionStatusChanged?.Invoke("Disconnected");
                        break;
                    }

                    MessageReceived?.Invoke(line);
                }
            }
            catch (Exception ex)
            {
                ConnectionStatusChanged?.Invoke($"Disconnected : {ex.Message}");
            }
            finally
            {
                Disconnect();
            }
        }

        public void Disconnect()
        {
            try
            {
                receiveCts?.Cancel();
            }
            catch { }

            try
            {
                reader?.Dispose();
                writer?.Dispose();
                tcpClient?.Close();
            }
            catch { }

            reader = null;
            writer = null;
            tcpClient = null;
        }
    }
}
