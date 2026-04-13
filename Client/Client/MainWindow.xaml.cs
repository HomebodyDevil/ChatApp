using Client.ViewModels;
using System.Collections.Specialized;
using System.Windows;

namespace Client
{
    /// <summary>
    /// MainWindow.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();

            var vm = new MainViewModel();
            DataContext = vm;

            vm.ChatLogs.CollectionChanged += ChatLogs_CollectionChanged;
        }

        private void ChatLogs_CollectionChanged(object sernder, NotifyCollectionChangedEventArgs e)
        {
            if (ChatLogListBox.Items.Count > 0)
            {
                ChatLogListBox.ScrollIntoView(
                    ChatLogListBox.Items[ChatLogListBox.Items.Count - 1]);
            }
        }

        private void MessageInputbox_PreviewKeyDown(object sender, System.Windows.Input.KeyEventArgs e)
        {
            if (e.Key == System.Windows.Input.Key.Enter && DataContext is MainViewModel vm)
            {
                if (vm.SendCommand.CanExecute(null))
                {
                    vm.SendCommand.Execute(null);
                    e.Handled = true;   // 이벤트가 전파되지 않음.(Preview = 터널링, 그냥 = 버블링)
                }
            }
        }
    }
}
