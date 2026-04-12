using Client.Helpers;
using Client.Services;
using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Windows;

namespace Client.ViewModels
{
    public class MainViewModel : INotifyPropertyChanged
    {
        private readonly ChatNetworkService networkService;

        private string serverIp = "127.0.0.1";
        private string serverPort = "9000";
        private string nickname = string.Empty;
        private string messageInput = string.Empty;
        private string connectionStatus = "Disconnected";

        public string ServerIp
        {
            get => serverIp;
            set
            {
                serverIp = value;
                OnPropertyChanged();
            }
        }

        public string ServerPort
        {
            get => serverPort;
            set
            {
                serverPort = value;
                OnPropertyChanged();
            }
        }

        public string Nickname
        {
            get => nickname;
            set
            {
                nickname = value;
                OnPropertyChanged();
                connectCommand.RaiseCanExecuteChanged();
            }
        }

        public string MessageInput
        {
            get => messageInput;
            set
            {
                messageInput = value;
                OnPropertyChanged();
                sendCommand.RaiseCanExecuteChanged();
            }
        }

        public string ConnectionStatus
        {
            get => connectionStatus;
            set
            {
                connectionStatus = value;
                OnPropertyChanged();
            }
        }

        public ObservableCollection<string> ChatLogs { get; } = new ObservableCollection<string>();
        public ObservableCollection<string> Users { get; } = new ObservableCollection<string>();

        private readonly RelayCommand connectCommand;
        public RelayCommand ConnectCommand => connectCommand;

        private readonly RelayCommand sendCommand;
        public RelayCommand SendCommand => sendCommand;

        private readonly RelayCommand refreshusersCommand;
        public RelayCommand RefreshUsersCommand => refreshusersCommand;

        public MainViewModel()
        {
            networkService = new ChatNetworkService();
            networkService.MessageReceived += OnMessageReceived;
            networkService.ConnectionStatusChanged += OnConnectionStatusChanged;

            connectCommand = new RelayCommand(
                async _ => await ConnectAsync(),
                _ => !string.IsNullOrEmpty(Nickname));

            sendCommand = new RelayCommand(
                async _ => await SendMessageAsync(),
                _ => !string.IsNullOrEmpty(messageInput));

            refreshusersCommand = new RelayCommand(
                async _ => await RefreshUserAsync(),
                _ => networkService.IsConnected);
        }

        private async Task ConnectAsync()
        {
            if (!int.TryParse(ServerPort, out int port))
            {
                AddChatLog("[System] Invalid port");
                return;
            }

            bool connected = await networkService.ConnectAsync(ServerIp, port);
            if (!connected)
            {
                return;
            }

            try
            {
                await networkService.SendAsync($"NICK|{Nickname}");
                await networkService.SendAsync("LIST");
            }
            catch (Exception ex)
            {
                AddChatLog($"[System] Failed to initialize connection : {ex.Message}");
            }
        }

        private async Task SendMessageAsync()
        {
            if (!networkService.IsConnected)
            {
                AddChatLog("[System] Not connected");
                return;
            }

            string message = MessageInput.Trim();
            if (string.IsNullOrEmpty(message))
            {
                return;
            }

            try
            {
                await networkService.SendAsync($"MSG|{message}");
                MessageInput = string.Empty;
            }
            catch(Exception ex)
            {
                AddChatLog($"[System] Send failed : {ex.Message}");
            }
        }

        private async Task RefreshUserAsync()
        {
            if (!networkService.IsConnected)
            {
                return;
            }

            try
            {
                await networkService.SendAsync("LIST");
            }
            catch(Exception ex)
            {
                AddChatLog($"[System] Failed to refresh users : {ex.Message}");
            }
        }

        private void OnMessageReceived(string message)
        {
            Application.Current.Dispatcher.Invoke(() =>
            {
                ParseServerMessage(message);
            });
        }

        private void OnConnectionStatusChanged(string status)
        {
            Application.Current.Dispatcher.Invoke(() =>
            {
                ConnectionStatus = status;
                refreshusersCommand.RaiseCanExecuteChanged();
            });
        }

        private void ParseServerMessage(string message)
        {
            string[] parts = message.Split('|');

            if (parts.Length == 0)
            {
                AddChatLog(message);
                return;
            }

            switch(parts[0])
            {
                case "SYSTEM":
                    if (parts.Length >= 2)
                    {
                        string systemMessage = string.Join("|", parts, 1, parts.Length - 1);
                        AddChatLog($"[Syste] {systemMessage}");
                    }
                    else
                    {
                        AddChatLog("[System]");
                    }
                    break;
                case "CHAT":
                    if (parts.Length >= 3)
                    {
                        string sender = parts[1];
                        string chatMessage = string.Join("|", parts, 2, parts.Length - 2);
                        AddChatLog($"{sender} : {chatMessage}");
                    }
                    else
                    {
                        AddChatLog(message);
                    }
                    break;
                case "USERS":
                    Users.Clear();
                    if (parts.Length >= 2 && !string.IsNullOrEmpty(parts[1]))
                    {
                        string[] users = parts[1].Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
                        foreach(string user in users)
                        {
                            Users.Add(user);
                        }
                    }
                    break;
                default:
                    AddChatLog(message);
                    break;

            }
        }

        private void AddChatLog(string log)
        {
            ChatLogs.Add(log);
        }

        public event PropertyChangedEventHandler PropertyChanged;

        private void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
