using System.Runtime.CompilerServices;

namespace Xenon {
	public abstract class Component {
		public Entity Entity { get; set; }
	}

	public class IdentityComponent : Component {

	}

	public class TransformComponent : Component {

		public Transform Transform {
			get {
				GetTransform_Native(Entity.id, out Transform result);
				return result;
			}

			set {
				SetTransform_Native(Entity.id, ref value);
			}
		}

		public Vector3 Position {
			get {
				GetPosition_Native(Entity.id, out Vector3 result);
				return result;
			}

			set {
				SetPosition_Native(Entity.id, ref value);
			}
		}

		public Vector3 Rotation {
			get {
				GetRotation_Native(Entity.id, out Vector3 result);
				return result;
			}

			set {
				SetRotation_Native(Entity.id, ref value);
			}
		}

		public Vector3 Scale {
			get {
				GetScale_Native(Entity.id, out Vector3 result);
				return result;
			}

			set {
				SetScale_Native(Entity.id, ref value);
			}
		}

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void GetTransform_Native(ulong entityID, out Transform outTransform);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SetTransform_Native(ulong entityID, ref Transform inTransform);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void GetPosition_Native(ulong entityID, out Vector3 outPosition);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SetPosition_Native(ulong entityID, ref Vector3 inPosition);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void GetRotation_Native(ulong entityID, out Vector3 outRotation);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SetRotation_Native(ulong entityID, ref Vector3 inRotation);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void GetScale_Native(ulong entityID, out Vector3 outScale);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SetScale_Native(ulong entityID, ref Vector3 inScale);
	}

	public class ScriptComponent : Component {

	}
}
