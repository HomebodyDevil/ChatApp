using System;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Controls;

namespace Client.Services
{
    internal class ChatNetworkService
    {
        private TcpClient tcpClient;
        private StreamReader reader;
        private StreamWriter writer;
        private CancellationTokenSource receiveCts;
        private bool isDisconnecting = false;

        public Action<string> MessageReceived;
        public Action<string> ConnectionStatusChanged;

        public bool IsConnected => tcpClient?.Connected ?? false;

        public async Task<bool> ConnectAsync(string ip, int port)
        {
            try
            {
                isDisconnecting = false;

                tcpClient = new TcpClient();
                await tcpClient.ConnectAsync(ip, port);

                // BOM이 붙어, 서버측에서 이상하게 해석하게 되는 현상이 있다.
                // 따라서, Encoding을 없애줬다.
                NetworkStream networkStream = tcpClient.GetStream();
                reader = new StreamReader(networkStream, new UTF8Encoding(false));
                writer = new StreamWriter(networkStream, new UTF8Encoding(false))
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
                        if (!isDisconnecting)
                            ConnectionStatusChanged?.Invoke("Disconnected");

                        break;
                    }

                    MessageReceived?.Invoke(line);
                }
            }
            catch (Exception ex)
            {
                if (!isDisconnecting)
                    ConnectionStatusChanged?.Invoke($"Disconnected : {ex.Message}");
            }
            finally
            {
                // Disconnect();
                Cleanup();
            }
        }

        public void Disconnect()
        {
            isDisconnecting = true;
            Cleanup();
            ConnectionStatusChanged?.Invoke("Disconnected");
        }

        private void Cleanup()
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
            receiveCts = null;
        }
    }
}
