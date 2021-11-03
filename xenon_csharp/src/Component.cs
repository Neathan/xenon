using System.Runtime.CompilerServices;

namespace Xenon {
	public abstract class Component {
		public Entity entity { get; set; }
	}

	public class IdentityComponent : Component {
		// TODO: Implement
	}

	public class TransformComponent : Component {

		public Transform transform {
			get {
				GetTransform_Native(entity.id, out Transform result);
				return result;
			}
			set {
				SetTransform_Native(entity.id, ref value);
			}
		}

		public Vector3 position {
			get {
				GetPosition_Native(entity.id, out Vector3 result);
				return result;
			}
			set {
				SetPosition_Native(entity.id, ref value);
			}
		}

		public Vector3 rotation {
			get {
				GetRotation_Native(entity.id, out Vector3 result);
				return result;
			}
			set {
				SetRotation_Native(entity.id, ref value);
			}
		}

		public Vector3 scale {
			get {
				GetScale_Native(entity.id, out Vector3 result);
				return result;
			}
			set {
				SetScale_Native(entity.id, ref value);
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
		// TODO: Implement
	}

	public class PointLightComponent : Component {
		public Vector3 color {
			get {
				GetColor_Native(entity.id, out Vector3 result);
				return result;
			}
			set {
				SetColor_Native(entity.id, ref value);
			}
		}

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void GetColor_Native(ulong entityID, out Vector3 outColor);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SetColor_Native(ulong entityID, ref Vector3 inColor);
	}
}
