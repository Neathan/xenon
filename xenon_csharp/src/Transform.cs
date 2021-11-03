using System.Runtime.InteropServices;

namespace Xenon {

	[StructLayout(LayoutKind.Sequential)]
	public class Transform {
		public Vector3 position;
		public Vector3 rotation;
		public Vector3 scale;
		public uint parent;
	}

}
