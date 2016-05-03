using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace QuickData.UX
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        Lib.QuickDataAPI qdAPI = new Lib.QuickDataAPI("QuickData.exe");

        public MainWindow()
        {
            InitializeComponent();
        }


        #region Voxel Tagging
        private void useExistingMatrix_Checked(object sender, RoutedEventArgs e)
        {
            
        }
        #endregion
    }
}
